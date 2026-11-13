import QtQuick

QtObject {
    id: theme

    // Harmonized strictly with user's KDE system theme (#474747) with pro studio contrast
    readonly property color bgWindow: "#474747"      // User's exact KDE system window background
    readonly property color bgCard: "#393939"        // Distinct technical surface
    readonly property color bgHeader: "#3e3e3e"      // Subtle darker header
    readonly property color bgTrackHeader: "#383838" // Track header
    readonly property color bgRowAlt: "#424242"      // Subtle alternating rows
    readonly property color bgRowBeat: "#4d4d4d"     // Beat start highlight
    readonly property color bgRowBar: "#565656"      // Measure/bar start highlight
    readonly property color bgWell: "#2c2c2c"        // Recessed well / inputs / meters
    readonly property color bgWellHover: "#353535"

    // High-visibility cursor & playhead
    readonly property color cursorRow: "#5a5a5a"     // Crisp visible cursor
    readonly property color playRow: "#2e5668"       // Professional active playhead glow
    readonly property color playAccent: "#27ae60"    // Clean KDE green
    readonly property color pauseRow: "#544626"      // Distinct amber glow for paused row
    readonly property color pauseAccent: "#f39c12"   // Warm amber for paused indicator
    readonly property color stopAccent: "#da4453"    // Clean KDE red

    // Crisp typography (no washed-out text)
    readonly property color textHeading: "#ffffff"
    readonly property color textMain: "#f0f2f5"
    readonly property color textMuted: "#b4b8be"
    readonly property color textDim: "#7e838b"

    // Vibrant accent for active controls
    readonly property color accent: "#3daee9"        // KDE Plasma Blue
    readonly property color accentHover: "#56bcf3"

    // Technical borders & buttons
    readonly property color border: "#565656"
    readonly property color borderLight: "#686868"
    readonly property color btnBg: "#363636"
    readonly property color btnHover: "#484848"
    readonly property color btnText: "#ffffff"

    // Technical tracker columns (clean engineering look)
    readonly property color colNote: "#5cd6e8"
    readonly property color colInst: "#92ebb5"
    readonly property color colVol: "#f7b731"
    readonly property color colFx: "#e0e2e6"
}
