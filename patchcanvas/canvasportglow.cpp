#include "canvasportglow.h"

START_NAMESPACE_PATCHCANVAS

extern Canvas canvas;

CanvasPortGlow::CanvasPortGlow(PortType port_type, QObject* parent) :
    QGraphicsDropShadowEffect(parent)
{
    setBlurRadius(12);
    setOffset(0, 0);

    if (port_type == PORT_TYPE_AUDIO)
      setColor(canvas.theme->line_audio_glow);
    else if (port_type == PORT_TYPE_MIDI)
      setColor(canvas.theme->line_midi_glow);
    else if (port_type == PORT_TYPE_OUTRO)
      setColor(canvas.theme->line_outro_glow);
}

END_NAMESPACE_PATCHCANVAS
