#!/usr/bin/env python3
import sys
import os
import signal
from PyQt6.QtWidgets import QApplication, QMainWindow, QFileDialog, QWidget
from PyQt6.QtGui import QAction, QKeySequence, QIcon, QGuiApplication
from PyQt6.QtQuickWidgets import QQuickWidget
from PyQt6.QtCore import QUrl, Qt, QObject, QMetaObject
from myms_bridge import MymsEngine

class PluginNativeHostWindow(QMainWindow):
    """
    Dedicated floating native host window for Linux VST3, CLAP, and VST2 plugins.
    Supports both native X11 sessions and Wayland sessions (via XWayland bridge).
    """
    def __init__(self, parent, engine, track, slot=None, is_fx=False, is_pre=False, on_close_cb=None):
        super().__init__(None, Qt.WindowType.Window)
        self.engine = engine
        self.track = track
        self.slot = slot
        self.is_fx = is_fx
        self.is_pre = is_pre
        self.on_close_cb = on_close_cb
        self.setWindowFlags(Qt.WindowType.Window)

        # Set title reflecting plugin name, format, and track context
        if is_pre:
            name = self.engine.getTrackPreFxName(track, slot)
            fmt = self.engine.getTrackPreFxFormatName(track, slot)
            self.title_str = f"myms — {name} [{fmt}] — Track #{track+1} Pre-FX {slot+1}"
        elif is_fx:
            name = self.engine.getTrackFxName(track, slot)
            fmt = self.engine.getTrackFxFormatName(track, slot)
            if track == -1:
                self.title_str = f"myms — {name} [{fmt}] — Master Bus FX {slot+1}"
            else:
                self.title_str = f"myms — {name} [{fmt}] — Track #{track+1} FX {slot+1}"
        else:
            name = self.engine.getTrackPluginName(track)
            fmt = self.engine.getTrackPluginFormatName(track)
            self.title_str = f"myms — {name} [{fmt}] — Track #{track+1} Plugin GUI"

        self.setWindowTitle(self.title_str)
        self.resize(520, 360)
        self.setStyleSheet("background-color: #232629; color: #f0f2f5;")

        self.x11 = None
        self.x11_dpy = None
        self.x11_win = None

        if QGuiApplication.platformName() != "wayland":
            # Native X11 session: use Qt's native window container
            self.container = QWidget(self)
            self.container.setAttribute(Qt.WidgetAttribute.WA_NativeWindow, True)
            self.container.setAttribute(Qt.WidgetAttribute.WA_PaintOnScreen, True)
            self.container.setStyleSheet("background-color: #232629;")
            self.setCentralWidget(self.container)

    def showEvent(self, event):
        super().showEvent(event)
        if QGuiApplication.platformName() == "wayland" and os.environ.get("DISPLAY"):
            # On Wayland, create a top-level X11 window on XWayland so X11 plugins embed cleanly
            try:
                import ctypes
                from ctypes import util
                x11_path = util.find_library("X11")
                if x11_path:
                    self.x11 = ctypes.cdll.LoadLibrary(x11_path)
                    self.x11_dpy = self.x11.XOpenDisplay(None)
                    if self.x11_dpy:
                        root = self.x11.XDefaultRootWindow(self.x11_dpy)
                        self.x11_win = self.x11.XCreateSimpleWindow(
                            self.x11_dpy, root, 100, 100, 520, 360, 1, 0, 0x232629
                        )
                        self.x11.XStoreName(self.x11_dpy, self.x11_win, self.title_str.encode("utf-8"))
                        self.x11.XMapWindow(self.x11_dpy, self.x11_win)
                        self.x11.XFlush(self.x11_dpy)
                        win_id = int(self.x11_win)
                    else:
                        win_id = int(self.winId())
                else:
                    win_id = int(self.winId())
            except Exception as e:
                print(f"[myms] X11 Wayland bridge initialization error: {e}")
                win_id = int(self.winId())
        else:
            win_id = int(self.container.winId())

        if self.is_pre:
            self.engine.openTrackPreFxNativeEditor(self.track, self.slot, win_id)
        elif self.is_fx:
            self.engine.openTrackFxNativeEditor(self.track, self.slot, win_id)
        else:
            self.engine.openTrackPluginNativeEditor(self.track, win_id)

    def closeEvent(self, event):
        if self.is_pre:
            self.engine.closeTrackPreFxNativeEditor(self.track, self.slot)
        elif self.is_fx:
            self.engine.closeTrackFxNativeEditor(self.track, self.slot)
        else:
            self.engine.closeTrackPluginNativeEditor(self.track)

        if self.x11 and self.x11_dpy and self.x11_win:
            try:
                self.x11.XDestroyWindow(self.x11_dpy, self.x11_win)
                self.x11.XCloseDisplay(self.x11_dpy)
            except Exception:
                pass
            self.x11_win = None
            self.x11_dpy = None

        if self.on_close_cb:
            self.on_close_cb()
        event.accept()

class AppWindow(QMainWindow):
    def __init__(self, engine):
        super().__init__()
        self.engine = engine
        self._plugin_gui_windows = {}
        self._update_window_title()
        self.resize(1120, 780)
        self.setMinimumSize(920, 600)

        self.engine.currentProjectPathChanged.connect(self._update_window_title)
        self.engine.requestOpenProject.connect(self._on_open_project)
        self.engine.requestSaveProject.connect(self._on_save_project)
        self.engine.requestSaveProjectAs.connect(self._on_save_project_as)
        self.engine.requestNewProject.connect(self._on_new_project)
        self.engine.requestOpenPluginDialog.connect(self._on_open_plugin_dialog)
        self.engine.requestOpenPluginGui.connect(self._on_open_plugin_gui)
        self.engine.requestOpenTrackFxDialog.connect(self._on_open_track_fx_dialog)
        self.engine.requestOpenTrackFxGui.connect(self._on_open_track_fx_gui)
        self.engine.requestOpenTrackPreFxDialog.connect(self._on_open_track_pre_fx_dialog)
        self.engine.requestOpenTrackPreFxGui.connect(self._on_open_track_pre_fx_gui)

        # Setup QQuickWidget for hardware-accelerated QML content
        self.quick_widget = QQuickWidget(self)
        self.quick_widget.setResizeMode(QQuickWidget.ResizeMode.SizeRootObjectToView)
        self.quick_widget.rootContext().setContextProperty("mymsEngine", self.engine)

        current_dir = os.path.dirname(os.path.abspath(__file__))
        qml_file = os.path.join(current_dir, "qml", "main.qml")
        self.quick_widget.setSource(QUrl.fromLocalFile(qml_file))

        self.setCentralWidget(self.quick_widget)

        # Setup single application menu bar (adapts natively to Global Menu or in-window)
        self._setup_menu_bar()

    def _setup_menu_bar(self):
        self.mb = self.menuBar()

        # --- File Menu ---
        file_menu = self.mb.addMenu("&File")

        act_new = QAction("&New Project", self)
        act_new.setShortcut(QKeySequence.StandardKey.New)
        act_new.triggered.connect(self._on_new_project)
        file_menu.addAction(act_new)

        act_open = QAction("&Open Project...", self)
        act_open.setShortcut(QKeySequence.StandardKey.Open)
        act_open.triggered.connect(self._on_open_project)
        file_menu.addAction(act_open)

        act_save = QAction("&Save Project", self)
        act_save.setShortcut(QKeySequence.StandardKey.Save)
        act_save.triggered.connect(self._on_save_project)
        file_menu.addAction(act_save)

        act_save_as = QAction("Save Project &As...", self)
        act_save_as.setShortcut(QKeySequence.StandardKey.SaveAs)
        act_save_as.triggered.connect(self._on_save_project_as)
        file_menu.addAction(act_save_as)

        file_menu.addSeparator()

        act_export = QAction("&Export Audio...", self)
        act_export.setShortcut(QKeySequence("Ctrl+E"))
        act_export.triggered.connect(self.engine.requestExportAudio)
        file_menu.addAction(act_export)

        file_menu.addSeparator()

        act_demo = QAction("&Load Demo Groove", self)
        act_demo.triggered.connect(self.engine.loadDemoGroove)
        file_menu.addAction(act_demo)

        file_menu.addSeparator()

        act_quit = QAction("&Quit", self)
        act_quit.setShortcut(QKeySequence.StandardKey.Quit)
        act_quit.triggered.connect(self.close)
        file_menu.addAction(act_quit)

        # --- Edit Menu ---
        edit_menu = self.mb.addMenu("&Edit")

        act_cut = QAction("Cu&t Chunk", self)
        act_cut.setShortcut(QKeySequence.StandardKey.Cut)
        act_cut.triggered.connect(self._on_cut)
        edit_menu.addAction(act_cut)

        act_copy = QAction("&Copy Chunk", self)
        act_copy.setShortcut(QKeySequence.StandardKey.Copy)
        act_copy.triggered.connect(self._on_copy)
        edit_menu.addAction(act_copy)

        act_paste = QAction("&Paste Chunk", self)
        act_paste.setShortcut(QKeySequence.StandardKey.Paste)
        act_paste.triggered.connect(self._on_paste)
        edit_menu.addAction(act_paste)

        act_delete = QAction("&Delete Chunk", self)
        act_delete.setShortcut(QKeySequence.StandardKey.Delete)
        act_delete.triggered.connect(self._on_delete)
        edit_menu.addAction(act_delete)

        act_select_all = QAction("Select &All Pattern", self)
        act_select_all.setShortcut(QKeySequence.StandardKey.SelectAll)
        act_select_all.triggered.connect(self._on_select_all)
        edit_menu.addAction(act_select_all)

        edit_menu.addSeparator()

        act_add_track_e = QAction("&Add Track", self)
        act_add_track_e.setShortcut(QKeySequence("Ctrl+T"))
        act_add_track_e.triggered.connect(self.engine.addTrack)
        edit_menu.addAction(act_add_track_e)

        act_clear_all = QAction("Clear All Tracks", self)
        act_clear_all.triggered.connect(self.engine.clearAllTracks)
        edit_menu.addAction(act_clear_all)

        edit_menu.addSeparator()

        act_trans_up = QAction("Transpose +1 Semitone", self)
        act_trans_up.setShortcut(QKeySequence("Ctrl+Up"))
        act_trans_up.triggered.connect(lambda: self.engine.transposeTrack(self.engine.selectedInstrument, 1))
        edit_menu.addAction(act_trans_up)

        act_trans_down = QAction("Transpose -1 Semitone", self)
        act_trans_down.setShortcut(QKeySequence("Ctrl+Down"))
        act_trans_down.triggered.connect(lambda: self.engine.transposeTrack(self.engine.selectedInstrument, -1))
        edit_menu.addAction(act_trans_down)

        act_oct_up = QAction("Transpose +1 Octave (+12)", self)
        act_oct_up.setShortcut(QKeySequence("Ctrl+Shift+Up"))
        act_oct_up.triggered.connect(lambda: self.engine.transposeTrack(self.engine.selectedInstrument, 12))
        edit_menu.addAction(act_oct_up)

        act_oct_down = QAction("Transpose -1 Octave (-12)", self)
        act_oct_down.setShortcut(QKeySequence("Ctrl+Shift+Down"))
        act_oct_down.triggered.connect(lambda: self.engine.transposeTrack(self.engine.selectedInstrument, -12))
        edit_menu.addAction(act_oct_down)

        # --- Playback Menu ---
        play_menu = self.mb.addMenu("&Playback")

        act_play = QAction("&Play / Pause", self)
        act_play.setShortcut(QKeySequence("Space"))
        act_play.triggered.connect(self.engine.togglePlay)
        play_menu.addAction(act_play)

        act_stop = QAction("&Stop", self)
        act_stop.setShortcut(QKeySequence("Esc"))
        act_stop.triggered.connect(self._on_escape_or_stop)
        play_menu.addAction(act_stop)

        act_song_mode = QAction("Toggle &Song / Pattern Mode", self)
        act_song_mode.setShortcut(QKeySequence("F5"))
        act_song_mode.triggered.connect(self.engine.toggleSongMode)
        play_menu.addAction(act_song_mode)

        # --- Tracks Menu ---
        track_menu = self.mb.addMenu("&Tracks")
        act_add_track = QAction("&Add New Track", self)
        act_add_track.setShortcut(QKeySequence("Ctrl+T"))
        act_add_track.triggered.connect(self.engine.addTrack)
        track_menu.addAction(act_add_track)

        act_remove_track = QAction("&Remove Selected Track", self)
        act_remove_track.setShortcut(QKeySequence("Ctrl+Shift+T"))
        act_remove_track.triggered.connect(self.engine.removeSelectedTrack)
        track_menu.addAction(act_remove_track)

        # --- Settings Menu ---
        settings_menu = self.mb.addMenu("&Settings")
        layout_menu = settings_menu.addMenu("Keyboard &Layout")

        act_layout_tr = QAction("Turkish Q Layout", self)
        act_layout_tr.setCheckable(True)
        act_layout_tr.setChecked(self.engine.keyboardLayout == "tr")
        act_layout_tr.triggered.connect(lambda: self.engine.setKeyboardLayout("tr"))
        layout_menu.addAction(act_layout_tr)

        act_layout_us = QAction("US QWERTY Layout", self)
        act_layout_us.setCheckable(True)
        act_layout_us.setChecked(self.engine.keyboardLayout == "us")
        act_layout_us.triggered.connect(lambda: self.engine.setKeyboardLayout("us"))
        layout_menu.addAction(act_layout_us)

        def _on_layout_changed(layout):
            act_layout_tr.setChecked(layout == "tr")
            act_layout_us.setChecked(layout == "us")
        self.engine.keyboardLayoutChanged.connect(_on_layout_changed)

    def _update_window_title(self, path=None):
        name = self.engine.projectName
        if name:
            self.setWindowTitle(f"myms — [{name}]")
        else:
            self.setWindowTitle("myms")

    def _get_tracker_view(self):
        if not hasattr(self, 'quick_widget'):
            return None
        root = self.quick_widget.rootObject()
        if not root:
            return None
        return root.findChild(QObject, "trackerView")

    def _on_copy(self):
        tracker = self._get_tracker_view()
        if tracker:
            if tracker.property("hasSelection"):
                st = tracker.property("selStartTrack")
                et = tracker.property("selEndTrack")
                sr = tracker.property("selStartRow")
                er = tracker.property("selEndRow")
                self.engine.copyRange(st, sr, et, er)
            else:
                st = tracker.property("selectedTrack")
                sr = tracker.property("selectedRow")
                self.engine.copyRange(st, sr, st, sr)

    def _on_cut(self):
        tracker = self._get_tracker_view()
        if tracker:
            if tracker.property("hasSelection"):
                st = tracker.property("selStartTrack")
                et = tracker.property("selEndTrack")
                sr = tracker.property("selStartRow")
                er = tracker.property("selEndRow")
                self.engine.cutRange(st, sr, et, er)
                tracker.setProperty("hasSelection", False)
            else:
                st = tracker.property("selectedTrack")
                sr = tracker.property("selectedRow")
                self.engine.cutRange(st, sr, st, sr)
            QMetaObject.invokeMethod(tracker, "gridModified")

    def _on_paste(self):
        tracker = self._get_tracker_view()
        if tracker:
            st = tracker.property("selectedTrack")
            sr = tracker.property("selectedRow")
            self.engine.pasteClipboard(st, sr)
            QMetaObject.invokeMethod(tracker, "gridModified")

    def _on_delete(self):
        tracker = self._get_tracker_view()
        if tracker:
            if tracker.property("hasSelection"):
                st = tracker.property("selStartTrack")
                et = tracker.property("selEndTrack")
                sr = tracker.property("selStartRow")
                er = tracker.property("selEndRow")
                self.engine.deleteRange(st, sr, et, er)
                tracker.setProperty("hasSelection", False)
            else:
                st = tracker.property("selectedTrack")
                sr = tracker.property("selectedRow")
                self.engine.deleteRange(st, sr, st, sr)
            QMetaObject.invokeMethod(tracker, "gridModified")

    def _on_select_all(self):
        tracker = self._get_tracker_view()
        if tracker:
            tracker.setProperty("selStartTrack", 0)
            tracker.setProperty("selStartRow", 0)
            tracker.setProperty("selEndTrack", self.engine.numTracks - 1)
            tracker.setProperty("selEndRow", self.engine.numRows - 1)
            tracker.setProperty("hasSelection", True)

    def _on_escape_or_stop(self):
        root = self.quick_widget.rootObject() if hasattr(self, 'quick_widget') else None
        if root:
            # Check modals in reverse order of z-index (topmost first)
            export_modal = root.findChild(QObject, "exportModal")
            if export_modal and export_modal.property("isOpen"):
                if not self.engine.isExporting:
                    export_modal.setProperty("isOpen", False)
                return
            cell_modal = root.findChild(QObject, "cellEditorModal")
            if cell_modal and cell_modal.property("isOpen"):
                cell_modal.setProperty("isOpen", False)
                return
            macro_modal = root.findChild(QObject, "macroModal")
            if macro_modal and macro_modal.property("isOpen"):
                macro_modal.setProperty("isOpen", False)
                return
            fx_modal = root.findChild(QObject, "fxModal")
            if fx_modal and fx_modal.property("isOpen"):
                fx_modal.setProperty("isOpen", False)
                return
            editor_modal = root.findChild(QObject, "editorModal")
            if editor_modal and editor_modal.property("isOpen"):
                editor_modal.setProperty("isOpen", False)
                return
            tracker_view = root.findChild(QObject, "trackerView")
            if tracker_view and tracker_view.property("hasSelection"):
                tracker_view.setProperty("hasSelection", False)
                return
        self.engine.stop()

    def _on_new_project(self):
        self.engine.newProject()

    def _on_open_project(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Open myms Project",
            os.path.expanduser("~"),
            "myms Project Files (*.myms);;All Files (*)"
        )
        if file_path:
            self.engine.openProject(file_path)

    def _on_save_project(self):
        if self.engine.currentProjectPath:
            self.engine.saveProject(self.engine.currentProjectPath)
        else:
            self._on_save_project_as()

    def _on_save_project_as(self):
        default_name = f"{self.engine.projectName if self.engine.projectName else 'Untitled'}.myms"
        default_path = os.path.join(os.path.expanduser("~"), default_name)
        file_path, _ = QFileDialog.getSaveFileName(
            self,
            "Save myms Project As",
            default_path,
            "myms Project Files (*.myms);;All Files (*)"
        )
        if file_path:
            if not file_path.endswith(".myms"):
                file_path += ".myms"
            self.engine.saveProject(file_path)

    def _on_open_plugin_dialog(self, track):
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            f"Load Plugin Instrument for Track #{track+1}",
            os.path.abspath("plugins"),
            "Audio Plugins (*.vst3 *.clap *.so);;All Files (*)"
        )
        if file_path:
            self.engine.loadTrackPlugin(track, file_path)

    def _on_open_plugin_gui(self, track):
        if not self.engine.isTrackPluginLoaded(track):
            return
        key = (track, "inst")
        win = self._plugin_gui_windows.get(key)
        if win is not None:
            try:
                win.show()
                win.raise_()
                win.activateWindow()
                return
            except Exception:
                pass

        def _cleanup():
            self._plugin_gui_windows.pop(key, None)

        win = PluginNativeHostWindow(self, self.engine, track, is_fx=False, on_close_cb=_cleanup)
        self._plugin_gui_windows[key] = win
        win.show()
        win.raise_()
        win.activateWindow()

    def _on_open_track_fx_dialog(self, track, slot):
        target_name = "Master Bus" if track == -1 else f"Track #{track+1}"
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            f"Select Insert Effect Plugin for {target_name}" + (f" (Slot {slot+1})" if slot >= 0 else ""),
            os.path.abspath("plugins"),
            "Audio Plugins (*.vst3 *.clap *.so);;All Files (*)"
        )
        if file_path:
            if slot < 0:
                self.engine.addTrackFx(track, file_path)
            else:
                self.engine.loadTrackFx(track, slot, file_path)

    def _on_open_track_fx_gui(self, track, slot):
        if not self.engine.isTrackFxLoaded(track, slot):
            return
        key = (track, f"fx_{slot}")
        win = self._plugin_gui_windows.get(key)
        if win is not None:
            try:
                win.show()
                win.raise_()
                win.activateWindow()
                return
            except Exception:
                pass

        def _cleanup():
            self._plugin_gui_windows.pop(key, None)

        win = PluginNativeHostWindow(self, self.engine, track, slot=slot, is_fx=True, on_close_cb=_cleanup)
        self._plugin_gui_windows[key] = win
        win.show()
        win.raise_()
        win.activateWindow()

    def _on_open_track_pre_fx_dialog(self, track, slot):
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            f"Select Pre-Instrument Effect Plugin for Track #{track+1}" + (f" (Slot {slot+1})" if slot >= 0 else ""),
            os.path.abspath("plugins"),
            "Audio Plugins (*.vst3 *.clap *.so);;All Files (*)"
        )
        if file_path:
            if slot < 0:
                self.engine.addTrackPreFx(track, file_path)
            else:
                self.engine.loadTrackPreFx(track, slot, file_path)

    def _on_open_track_pre_fx_gui(self, track, slot):
        if not self.engine.isTrackPreFxLoaded(track, slot):
            return
        key = (track, f"pre_fx_{slot}")
        win = self._plugin_gui_windows.get(key)
        if win is not None:
            try:
                win.show()
                win.raise_()
                win.activateWindow()
                return
            except Exception:
                pass

        def _cleanup():
            self._plugin_gui_windows.pop(key, None)

        win = PluginNativeHostWindow(self, self.engine, track, slot=slot, is_fx=False, is_pre=True, on_close_cb=_cleanup)
        self._plugin_gui_windows[key] = win
        win.show()
        win.raise_()
        win.activateWindow()

    def closeEvent(self, event):
        for win in list(self._plugin_gui_windows.values()):
            try:
                win.close()
            except Exception:
                pass
        self._plugin_gui_windows.clear()
        self.engine.close()
        event.accept()

def main():
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    
    QApplication.setApplicationName("myms")
    QApplication.setDesktopFileName("myms")

    app = QApplication(sys.argv)

    try:
        engine = MymsEngine()
    except Exception as e:
        print(f"[myms] Failed to initialize audio engine: {e}")
        return 1

    win = AppWindow(engine)
    win.show()

    ret = app.exec()
    return ret

if __name__ == "__main__":
    sys.exit(main())
