#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "get.h"

int main() {
  const char* mute_command = "pactl set-sink-mute @DEFAULT_SINK@ 1";
  const char* unmute_command = "pactl set-sink-mute @DEFAULT_SINK@ 0";
  bool ad_playing = false;
  const unsigned int check_interval = 10;

  while (1) {
    char* type = get_current_playing_type();
    if (type == NULL) {
      printf(
          "No playback detected, double check that you're playing. RETRYING");
      sleep(check_interval);
      continue;
    }
    printf("Currently playing an %s... Ad status is %s\n", type, ad_playing ? "true" : "false");
    // if ad is not playing, check if ad is playing
    if (!ad_playing && strcmp(type, "ad") == 0) {
      ad_playing = true;
      int result = system(mute_command);
      if (result == 0) {
        printf("Laptop muted.\n");
      } else {
        fprintf(stderr, "Error Exit code: %d\n", result);
      }
      // ad is playing, check if track is playing to unmute
    } else if (ad_playing && strcmp(type, "track") == 0) {
      ad_playing = false;
      int result = system(unmute_command);
      if (result == 0) {
        printf("Laptop unmuted.\n");
      } else {
        fprintf(stderr, "Error Exit code: %d\n", result);
      }
    }

    free(type);
    sleep(check_interval);
  }
  return 0;
}
