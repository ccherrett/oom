#include "canvasportglow.h"

CanvasPortGlow::CanvasPortGlow()
{
    setBlurRadius(12);
    setOffset(0, 0);
}

void CanvasPortGlow::setPortType(PatchCanvas::PortType port_type, Theme* theme)
{
    if (port_type == PatchCanvas::PORT_TYPE_AUDIO)
      setColor(theme->line_audio_glow);
    else if (port_type == PatchCanvas::PORT_TYPE_MIDI)
      setColor(theme->line_midi_glow);
    else if (port_type == PatchCanvas::PORT_TYPE_OUTRO)
      setColor(theme->line_outro_glow);
}
