#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QPen>
#include <QFont>
#include <QString>

class Theme
{
public:
    enum PortType {
        THEME_PORT_SQUARE  = 0,
        THEME_PORT_POLYGON = 1
    };

    enum List {
        THEME_MODERN_DARK  = 0,
        THEME_CLASSIC_DARK = 1,
        THEME_MAX = 2
    };

    Theme(List id);

    // Canvas
    QString name;

    // Boxes
    QColor canvas_bg;
    QPen box_pen;
    QPen box_pen_sel;
    QColor box_bg_1;
    QColor box_bg_2;
    QColor box_shadow;
    QPen box_text;
    QString box_font_name;
    int box_font_size;
    QFont::Weight box_font_state;

    // Ports
    QPen port_audio_jack_pen;
    QPen port_audio_jack_pen_sel;
    QPen port_midi_jack_pen;
    QPen port_midi_jack_pen_sel;
    QPen port_midi_a2j_pen;
    QPen port_midi_a2j_pen_sel;
    QPen port_midi_alsa_pen;
    QPen port_midi_alsa_pen_sel;
    QColor port_audio_jack_bg;
    QColor port_audio_jack_bg_sel;
    QColor port_midi_jack_bg;
    QColor port_midi_jack_bg_sel;
    QColor port_midi_a2j_bg;
    QColor port_midi_a2j_bg_sel;
    QColor port_midi_alsa_bg;
    QColor port_midi_alsa_bg_sel;
    QPen port_text;
    QString port_font_name;
    int port_font_size;
    QFont::Weight port_font_state;
    PortType port_mode;

    // Lines
    QColor line_audio_jack;
    QColor line_audio_jack_sel;
    QColor line_audio_jack_glow;
    QColor line_midi_jack;
    QColor line_midi_jack_sel;
    QColor line_midi_jack_glow;
    QColor line_midi_a2j;
    QColor line_midi_a2j_sel;
    QColor line_midi_a2j_glow;
    QColor line_midi_alsa;
    QColor line_midi_alsa_sel;
    QColor line_midi_alsa_glow;
    QPen rubberband_pen;
    QColor rubberband_brush;

    static List getDefaultTheme();
    static QString getThemeName(List id);
};

#endif // THEME_H
