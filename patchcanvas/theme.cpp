#include "theme.h"

#include <cstdlib>
#include <cstring>

Theme::Theme()
{
#if 0 /* Modern Black theme */

    // Name this theme
    name = "Modern Dark";

    // Canvas
    canvas_bg = QColor(0, 0, 0);

    // Boxes
    box_pen = QPen(QColor(76,77,78), 1, Qt::SolidLine);
    box_pen_sel = QPen(QColor(206,207,208), 1, Qt::DashLine);
    box_bg_1 = QColor(32,34,35);
    box_bg_2 = QColor(43,47,48);
    box_shadow = QColor(89,89,89,180);

    box_text = QPen(QColor(240,240,240), 0);
    box_font_name = "Deja Vu Sans";
    box_font_size = 8;
    box_font_state = QFont::Bold;

    // Ports
    port_audio_pen = QPen(QColor(63,90,126), 1);
    port_audio_pen_sel = QPen(QColor(93,120,156), 1);
    port_midi_pen = QPen(QColor(159,44,42), 1);
    port_midi_pen_sel = QPen(QColor(189,74,72), 1);
    port_outro_pen = QPen(QColor(93,141,46), 1);
    port_outro_pen_sel = QPen(QColor(123,171,76), 1);

    port_audio_bg = QColor(35,61,99);
    port_audio_bg_sel = QColor(85,111,149);
    port_midi_bg = QColor(120,15,16);
    port_midi_bg_sel = QColor(170,65,66);
    port_outro_bg = QColor(64,112,18);
    port_outro_bg_sel = QColor(114,162,68);

    port_text = QPen(QColor(250,250,250), 0);
    port_font_name = "Deja Vu Sans";
    port_font_size = 8;
    port_font_state = QFont::Normal;
    port_mode = THEME_PORT_POLYGON;

    // Lines
    line_audio = QColor(63,90,126);
    line_audio_sel = QColor(63+70,90+70,126+70);
    line_audio_glow = QColor(0,0,255);
    line_midi = QColor(159,44,42);
    line_midi_sel = QColor(159+70,44+70,42+70);
    line_midi_glow = QColor(255,0,0);
    line_outro = QColor(93,141,46);
    line_outro_sel = QColor(93+70,141+70,46+70);
    line_outro_glow = QColor(0,255,0);

    rubberband_pen = QPen(QColor(206,207,208), 1, Qt::SolidLine),
            rubberband_brush = QColor(76,77,78,100);

#else /* Classic Dark theme */

    // Name this theme
    name = "Classic Dark";

    // Canvas
    canvas_bg = QColor(0,0,0);

    // Boxes
    box_pen = QPen(QColor(147-70,151-70,143-70), 2, Qt::SolidLine);
    box_pen_sel = QPen(QColor(147,151,143), 2, Qt::DashLine);
    box_bg_1 = QColor(30,34,36);
    box_bg_2 = QColor(30,34,36);
    box_shadow = QColor(89,89,89,180);

    box_text = QPen(QColor(255,255,255), 0);
    box_font_name = "Sans";
    box_font_size = 9;
    box_font_state = QFont::Normal;

    // Ports
    port_audio_pen = QPen(QColor(35,61,99), 0);
    port_audio_pen_sel = QPen(QColor(255,0,0), 0);
    port_midi_pen = QPen(QColor(120,15,16), 0);
    port_midi_pen_sel = QPen(QColor(255,0,0), 0);
    port_outro_pen = QPen(QColor(63,112,19), 0);
    port_outro_pen_sel = QPen(QColor(255,0,0), 0);

    port_audio_bg = QColor(35,61,99);
    port_audio_bg_sel = QColor(255,0,0);
    port_midi_bg = QColor(120,15,16);
    port_midi_bg_sel = QColor(255,0,0);
    port_outro_bg = QColor(63,112,19);
    port_outro_bg_sel = QColor(255,0,0);

    port_text = QPen(QColor(250,250,250), 0);
    port_font_name = "Sans";
    port_font_size = 8;
    port_font_state = QFont::Normal;
    port_mode = THEME_PORT_SQUARE;

    // Lines
    line_audio = QColor(53,78,116);
    line_audio_sel = QColor(255,0,0);
    line_audio_glow = QColor(255,0,0);
    line_midi = QColor(139,32,32);
    line_midi_sel = QColor(255,0,0);
    line_midi_glow = QColor(255,0,0);
    line_outro = QColor(81,130,36);
    line_outro_sel = QColor(255,0,0);
    line_outro_glow = QColor(255,0,0);

    rubberband_pen = QPen(QColor(147,151,143), 2, Qt::SolidLine);
    rubberband_brush = QColor(35,61,99,100);
#endif
}
