#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure to store HTTP response data dynamically
struct Memory {
  char* response;
  size_t size;
};

// Function which reallocates memory to fit incoming data chunks
static size_t ResponseCallback(void* contents, size_t size, size_t nmemb,
                               void* userp) {
  size_t totalSize = size * nmemb;
  struct Memory* mem = (struct Memory*)userp;

  // Reallocate memory to fit new data
  char* ptr = realloc(mem->response, mem->size + totalSize + 1);
  if (!ptr) {
    return 0;  // Out of memory
  }

  // Copy new data to buffer
  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), contents, totalSize);
  mem->size += totalSize;
  mem->response[mem->size] = '\0';

  return totalSize;
}

// Automated token renewal
int refresh_access_token(const char* client_id, const char* client_secret,
                         const char* refresh_token, char* new_access_token,
                         size_t token_size) {
  CURL* curl = NULL;
  CURLcode res = CURLE_OK;
  struct Memory chunk;

  chunk.response = malloc(1);
  chunk.size = 0;

  curl = curl_easy_init();
  if (!curl) {
    free(chunk.response);
    return 0;
  }

  // Spotify token endpoint
  curl_easy_setopt(curl, CURLOPT_URL, "https://accounts.spotify.com/api/token");

  // Prepare POST data with refresh token
  char post_data[1024];
  snprintf(post_data, sizeof(post_data),
           "grant_type=refresh_token&refresh_token=%s", refresh_token);

  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

  // Set up basic authentication (client_id:client_secret)
  char credentials[512];
  snprintf(credentials, sizeof(credentials), "%s:%s", client_id, client_secret);
  curl_easy_setopt(curl, CURLOPT_USERPWD, credentials);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ResponseCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);

  // Execute request
  res = curl_easy_perform(curl);

  int success = 0;
  if (res == CURLE_OK) {
    // Parse JSON response to extract new access token
    cJSON* json = cJSON_Parse(chunk.response);
    if (json) {
      cJSON* access_token_json =
          cJSON_GetObjectItemCaseSensitive(json, "access_token");
      if (cJSON_IsString(access_token_json) && access_token_json->valuestring) {
        strncpy(new_access_token, access_token_json->valuestring,
                token_size - 1);
        new_access_token[token_size - 1] = '\0';

        // Save new token to file
        FILE* fp = fopen("token.txt", "w");
        if (fp) {
          fprintf(fp, "%s", new_access_token);
          fclose(fp);
          success = 1;
          printf("Access token refreshed successfully!\n");
        }
      }
      cJSON_Delete(json);
    }
  } else {
    fprintf(stderr, "Token refresh failed: %s\n", curl_easy_strerror(res));
  }

  free(chunk.response);
  curl_easy_cleanup(curl);

  return success;
}

// Getting the current playing type
char* get_current_playing_type(void) {
  puts("Starting getting current playing type");

  // Read access token from file
  char access_token[512];
  FILE* filePointer = fopen("token.txt", "r");
  if (!filePointer) {
    fprintf(stderr, "Failed to open token.txt\n");
    return NULL;
  }
  fgets(access_token, sizeof(access_token), filePointer);
  fclose(filePointer);

  // Remove newline if present
  access_token[strcspn(access_token, "\n")] = '\0';

  // Initialize curl and memory buffer
  CURL* curl = NULL;
  CURLcode res = CURLE_OK;
  struct Memory chunk;

  chunk.response = malloc(1);
  chunk.size = 0;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();

  if (!curl) {
    printf("Failed to init curl.\n");
    free(chunk.response);
    curl_global_cleanup();
    return NULL;
  }

  // Setup HTTP Request
  const char* url = "https://api.spotify.com/v1/me/player";
  printf("URL: %s\n", url);

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  struct curl_slist* headers = NULL;
  char auth_header[1024];
  snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s",
           access_token);

  headers = curl_slist_append(headers, auth_header);
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  // Setup response handling
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ResponseCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

  long http_code = 0;

  // Execute HTTP request
  res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
  } else {
    // Get HTTP status code
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    printf("HTTP Status Code: %ld\n", http_code);

    // Display response or handle no-content case
    // printf("Response (%zu bytes):\n", chunk.size);
    if (chunk.size > 0) {
    //   puts(chunk.response);
    } else if (http_code == 204) {
      printf("No content (204): Nothing is currently playing.\n");
    }
  }

  // Parse JSON Response
  cJSON* json = cJSON_Parse(chunk.response);
  if (!json) {
    fprintf(stderr, "JSON parse error\n");
    free(chunk.response);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return NULL;
  }

  // Get playback type
  char* playing_type_string = NULL;
  cJSON* playing_type =
      cJSON_GetObjectItemCaseSensitive(json, "currently_playing_type");
  if (cJSON_IsString(playing_type) && (playing_type->valuestring != NULL)) {
    // Allocate and copy the playback type string
    playing_type_string = malloc(strlen(playing_type->valuestring) + 1);
    // switched to strncpy for safety
    strncpy(playing_type_string, playing_type->valuestring, strlen(playing_type->valuestring));
    playing_type_string[strlen(playing_type->valuestring)] = '\0';
    printf("Currently playing type: %s\n", playing_type_string);
  } else {
    printf("Key not found or not a string\n");
  }

  cJSON_Delete(json);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  free(chunk.response);
  curl_global_cleanup();

  return playing_type_string;
}
