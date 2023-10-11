#include "../defs.h"
#include <pulse/pulseaudio.h>
#include <stdio.h>
#include <string.h>
#include "volume.h"
#include "../display.h"

static void context_state_callback(pa_context *c, void *userdata);
static void sink_info_callback(pa_context *c, const pa_sink_info *i, int idx,
                               void *userdata);
static void source_info_callback(pa_context *c, const pa_source_info *i,
                                 int idx, void *userdata);
pa_mainloop *volume_loop = NULL;
pa_mainloop *mic_loop = NULL;
void pulse_loop(int pulse_event, pa_mainloop *loop) {
  pa_mainloop_api *api;
  pa_context *context = NULL;

  // Create a mainloop API and mainloop
  loop = pa_mainloop_new();
  api = pa_mainloop_get_api(loop);

  // Create a new PulseAudio context
  context = pa_context_new(api, "PulseAudio Example");

  // Set up context state callback
  pa_context_set_state_callback(context, context_state_callback, &pulse_event);

  // Connect to the PulseAudio server
  pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);

  // Start the mainloop to handle events
  pa_mainloop_run(loop, NULL);

  // Cleanup
  pa_context_unref(context);
  pa_mainloop_free(loop);
}

static void event_callback_sink(pa_context *c,
                                enum pa_subscription_event_type,
                                unsigned int, void *) {
  pa_context_get_sink_info_by_index(c, 0, sink_info_callback, NULL);
}

static void event_callback_source(pa_context *c,
                                  enum pa_subscription_event_type,
                                  unsigned int, void *) {
  pa_context_get_source_info_by_index(c, 0, source_info_callback, NULL);
}

static void context_state_callback(pa_context *c, void *userdata) {
  pa_context_state_t state = pa_context_get_state(c);

  if (state == PA_CONTEXT_READY) {
    int i;
    if (*((int *)userdata) == SINK) {
      pa_context_get_sink_info_by_index(c, 0, sink_info_callback, NULL);
      i = 0;
      pa_context_set_subscribe_callback(c, event_callback_sink, &i);
      pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK, NULL, NULL);
    } else {
      pa_context_get_source_info_by_index(c, 0, source_info_callback, NULL);
      i = 1;
      pa_context_set_subscribe_callback(c, event_callback_source, &i);
      pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SOURCE, NULL, NULL);
    }
  }
}

static void sink_info_callback(pa_context *, const pa_sink_info *i, int,
                               void *) {
  if (i) {
    float vol = (float)pa_cvolume_avg(&(i->volume)) / (float)PA_VOLUME_NORM;
    if (i->mute) {
      strcpy(modules[volume].string, "󰝟");
    } else {
      vol = vol * 100.0F;
      if (vol >= 60) {
        sprintf(modules[volume].string, "󰕾 %.f%%", vol);
      } else if (vol >= 30) {
        sprintf(modules[volume].string, "󰖀 %.f%%", vol);
      } else {
        sprintf(modules[volume].string, "󰕿 %.f%%", vol);
      }
    }
    display_modules(modules[volume].position);
  }
}

static void source_info_callback(pa_context *, const pa_source_info *i,
                                 int, void *) {
  if (i) {
    float vol = (float)pa_cvolume_avg(&(i->volume)) / (float)PA_VOLUME_NORM;
    if (i->mute) {
      strcpy(modules[mic].string, "󰍭");
    } else {
      sprintf(modules[mic].string, "󰍮 %.f%%", (vol * 100.0F));
    }
    display_modules(modules[mic].position);
  }
}

void *volume_update(void *) {
  pthread_mutex_lock(&mutex);
  modules[volume].string = (char *)malloc((VOLUME_BUFFER * sizeof(char)));
  modules[volume].string[0] = '\0';
  pthread_mutex_unlock(&mutex);
  pulse_loop(SINK, volume_loop);
  free(modules[volume].string);
  return NULL;
}

void *mic_update(void *) {
  pthread_mutex_lock(&mutex);
  modules[mic].string = (char *)malloc((MIC_BUFFER * sizeof(char)));
  modules[mic].string[0] = '\0';
  pthread_mutex_unlock(&mutex);
  pulse_loop(SOURCE, mic_loop);
  free(modules[mic].string);
  return NULL;
}