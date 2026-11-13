#!/usr/bin/env python3
import sys
import os
import signal
from PyQt6.QtWidgets import QApplication, QMainWindow, QFileDialog
from PyQt6.QtGui import QAction, QKeySequence, QIcon
from PyQt6.QtQuickWidgets import QQuickWidget
from PyQt6.QtCore import QUrl
from myms_bridge import MymsEngine

class AppWindow(QMainWindow):
    def __init__(self, engine):
        super().__init__()
        self.engine = engine
        self._update_window_title()
        self.resize(1120, 780)
        self.setMinimumSize(920, 600)

        self.engine.currentProjectPathChanged.connect(self._update_window_title)
        self.engine.requestOpenProject.connect(self._on_open_project)
        self.engine.requestSaveProject.connect(self._on_save_project)
        self.engine.requestSaveProjectAs.connect(self._on_save_project_as)
        self.engine.requestNewProject.connect(self._on_new_project)

        # Setup standard Qt native QMenuBar (100% KDE Global Menu compatible)
        self._setup_native_menu()

        # Setup QQuickWidget for hardware-accelerated QML content
        self.quick_widget = QQuickWidget(self)
        self.quick_widget.setResizeMode(QQuickWidget.ResizeMode.SizeRootObjectToView)
        self.quick_widget.rootContext().setContextProperty("mymsEngine", self.engine)

        current_dir = os.path.dirname(os.path.abspath(__file__))
        qml_file = os.path.join(current_dir, "qml", "main.qml")
        self.quick_widget.setSource(QUrl.fromLocalFile(qml_file))

        self.setCentralWidget(self.quick_widget)

    def _setup_native_menu(self):
        mb = self.menuBar()

        # --- File Menu ---
        file_menu = mb.addMenu("&File")

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

        act_demo = QAction("&Load Demo Groove", self)
        act_demo.triggered.connect(self.engine.loadDemoGroove)
        file_menu.addAction(act_demo)

        file_menu.addSeparator()

        act_quit = QAction("&Quit", self)
        act_quit.setShortcut(QKeySequence.StandardKey.Quit)
        act_quit.triggered.connect(self.close)
        file_menu.addAction(act_quit)

        # --- Edit Menu ---
        edit_menu = mb.addMenu("&Edit")

        act_add_track = QAction("&Add Track", self)
        act_add_track.setShortcut(QKeySequence("Ctrl+T"))
        act_add_track.triggered.connect(self.engine.addTrack)
        edit_menu.addAction(act_add_track)

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
        play_menu = mb.addMenu("&Playback")

        act_play = QAction("&Play / Pause", self)
        act_play.setShortcut(QKeySequence("Space"))
        act_play.triggered.connect(self.engine.togglePlay)
        play_menu.addAction(act_play)

        act_stop = QAction("&Stop", self)
        act_stop.setShortcut(QKeySequence("Esc"))
        act_stop.triggered.connect(self.engine.stop)
        play_menu.addAction(act_stop)

        act_song_mode = QAction("Toggle &Song / Pattern Mode", self)
        act_song_mode.setShortcut(QKeySequence("Ctrl+M"))
        act_song_mode.triggered.connect(self.engine.toggleSongMode)
        play_menu.addAction(act_song_mode)

        play_menu.addSeparator()

        act_bpm_up = QAction("Increase Tempo (+5 BPM)", self)
        act_bpm_up.setShortcut(QKeySequence("Alt+="))
        act_bpm_up.triggered.connect(lambda: self.engine.setBpm(self.engine.bpm + 5))
        play_menu.addAction(act_bpm_up)

        act_bpm_down = QAction("Decrease Tempo (-5 BPM)", self)
        act_bpm_down.setShortcut(QKeySequence("Alt+-"))
        act_bpm_down.triggered.connect(lambda: self.engine.setBpm(self.engine.bpm - 5))
        play_menu.addAction(act_bpm_down)

        # --- Settings Menu ---
        settings_menu = mb.addMenu("&Settings")
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

        # --- Track & Instrument Menu ---
        track_menu = mb.addMenu("&Tracks")
        act_add_track = QAction("&Add New Track", self)
        act_add_track.setShortcut(QKeySequence("Ctrl+T"))
        act_add_track.triggered.connect(self.engine.addTrack)
        track_menu.addAction(act_add_track)
        track_menu.addSeparator()

        for i in range(8):
            act_inst = QAction(f"Select Track / Instrument {i+1}", self)
            act_inst.setShortcut(QKeySequence(f"Ctrl+{i+1}"))
            idx = i
            act_inst.triggered.connect(lambda checked=False, target=idx: self.engine.setSelectedInstrument(target))
            track_menu.addAction(act_inst)

    def _update_window_title(self, path=None):
        name = self.engine.projectName
        if name:
            self.setWindowTitle(f"myms — [{name}]")
        else:
            self.setWindowTitle("myms")

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

    def closeEvent(self, event):
        self.engine.close()
        event.accept()

def main():
    signal.signal(signal.SIGINT, signal.SIG_DFL)

    # Force Qt to use native KDE platform theme for global menus and styling
    os.environ["QT_QPA_PLATFORMTHEME"] = "kde"
    
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
