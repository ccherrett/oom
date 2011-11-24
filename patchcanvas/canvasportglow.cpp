#include "canvasportglow.h"

START_NAMESPACE_PATCHCANVAS

extern Canvas canvas;

CanvasPortGlow::CanvasPortGlow(PortType port_type, QObject* parent) :
    QGraphicsDropShadowEffect(parent)
{
    setBlurRadius(12);
    setOffset(0, 0);

    if (port_type == PORT_TYPE_AUDIO_JACK)
      setColor(canvas.theme->line_audio_jack_glow);
    else if (port_type == PORT_TYPE_MIDI_JACK)
      setColor(canvas.theme->line_midi_jack_glow);
    else if (port_type == PORT_TYPE_MIDI_A2J)
      setColor(canvas.theme->line_midi_a2j_glow);
    else if (port_type == PORT_TYPE_MIDI_ALSA)
      setColor(canvas.theme->line_midi_alsa_glow);
}

END_NAMESPACE_PATCHCANVAS
