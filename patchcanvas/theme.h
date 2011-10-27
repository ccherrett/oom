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
    QPen port_audio_pen;
    QPen port_audio_pen_sel;
    QPen port_midi_pen;
    QPen port_midi_pen_sel;
    QPen port_outro_pen;
    QPen port_outro_pen_sel;
    QColor port_audio_bg;
    QColor port_audio_bg_sel;
    QColor port_midi_bg;
    QColor port_midi_bg_sel;
    QColor port_outro_bg;
    QColor port_outro_bg_sel;
    QPen port_text;
    QString port_font_name;
    int port_font_size;
    QFont::Weight port_font_state;
    PortType port_mode;

    // Lines
    QColor line_audio;
    QColor line_audio_sel;
    QColor line_audio_glow;
    QColor line_midi;
    QColor line_midi_sel;
    QColor line_midi_glow;
    QColor line_outro;
    QColor line_outro_sel;
    QColor line_outro_glow;
    QPen rubberband_pen;
    QColor rubberband_brush;

    static List getDefaultTheme();
    static QString getThemeName(List id);
};

#endif // THEME_H
