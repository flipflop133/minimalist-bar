#include <pulse/pulseaudio.h>
void *volume_update(void *arg);
void *mic_update(void *arg);
#define VOLUME_BUFFER 10
#define MIC_BUFFER 10
extern pa_mainloop *volume_loop;
extern pa_mainloop *mic_loop;
enum pulse_events { SINK, SOURCE };