#include <cjson/cJSON.h>
#include <fyers_api.h>
#include <fyers_model.h>
#include <fyers_session.h>
#include <stdio.h>
#include <string.h>

fyers_session_t *get_fyers_session();
fyers_model_t *get_fyers_model(fyers_session_t *session);

int main(void) {
  fyers_session_t *session = get_fyers_session();
  if (session == NULL) {
    fprintf(stderr, "Error: Unable to find sessions\n");
    return 1;
  }
  fyers_model_t *model = get_fyers_model(session);

  if (model == NULL) {
    fyers_session_destroy(session);
    fyers_cleanup();
    fprintf(stderr, "Error: Unable to find Fyers Models\n");
    return 1;
  }
  puts("ready for take off");
  fyers_model_destroy(model);
  fyers_session_destroy(session);
  fyers_cleanup();

  return 0;
}

fyers_session_t *get_fyers_session() {
  FILE *file_read_ptr;
  char client_id[256];
  char redirect_uri[1024];
  char secret_key[256];

  if ((file_read_ptr = fopen("./data/fyers_credentials.txt", "r")) == NULL) {
    puts("Enter credentials: client_id redirect_uri secret_key");
    scanf("%254s %1023s %254s", client_id, redirect_uri, secret_key);

    puts("taken");
    // save for later use
    FILE *file_write_ptr;
    if ((file_write_ptr = fopen("./data/fyers_credentials.txt", "w")) == NULL) {
      fprintf(
          stderr,
          "File or directory (data/fyers_credentials.txt) couldnt be created");
      return NULL;
    }
    fprintf(file_write_ptr, "%s %s %s", client_id, redirect_uri, secret_key);

    fclose(file_write_ptr);
  } else {
    fscanf(file_read_ptr, "%254s %1023s %254s", client_id, redirect_uri,
           secret_key);
    fclose(file_read_ptr);
  }

  fyers_session_t *session =
      fyers_session_create(client_id, redirect_uri, secret_key);
  return session;
}

fyers_model_t *get_fyers_model(fyers_session_t *session) {
  char buffer[1024] = {0};

  fyers_model_t *model;
  FILE *file_read_ptr;

  puts("Started with session");
  if ((file_read_ptr = fopen("./data/access_token.txt", "r")) == NULL) {
    generate_authcode(session);

    puts("Enter auth code:");
    scanf("%1022s", buffer);
    fyers_session_set_authcode(session, buffer);
    fyers_response_t *token_response = generate_token(session);
    printf("resp: data-> %s \n", token_response->data);
    cJSON *token = cJSON_Parse(token_response->data);
    if (token == NULL) {
      const char *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL) {
        fprintf(stderr, "Error: No Token %s\n", error_ptr);
        cJSON_Delete(token);
        fyers_response_destroy(token_response);
        return NULL;
      }
    }
    const char *access_token_string = NULL;
    const cJSON *acess_token_cjson =
        cJSON_GetObjectItemCaseSensitive(token, "access_token");
    if (!cJSON_IsString(acess_token_cjson)) {
      fprintf(stderr, "token response is not a string");
      cJSON_Delete(token);
      fyers_response_destroy(token_response);
      return NULL;
    } else {
      access_token_string = acess_token_cjson->valuestring;
    }

    const char *client_id = fyers_session_get_client_id(session);
    model = fyers_model_create(client_id, access_token_string, false, NULL,
                               FYERS_LOG_INFO);

    if (!model) {
      cJSON_Delete(token);
      fyers_response_destroy(token_response);
      fprintf(stderr, "Failed to create model");
      return NULL;
    }

    // save for later use
    FILE *file_write_ptr;
    if ((file_write_ptr = fopen("./data/access_token.txt", "w")) == NULL) {
      fprintf(stderr,
              "File or directory (data/access_token.txt) couldnt be created");
      return NULL;
    }
    fprintf(file_write_ptr, "%s", access_token_string);
    cJSON_Delete(token);
    fyers_response_destroy(token_response);
    fclose(file_write_ptr);
  } else {
    memset(buffer, 0, strlen(buffer));
    fscanf(file_read_ptr, "%1023s", buffer);
    const char *client_id = fyers_session_get_client_id(session);
    const char *access_token = buffer;
    model = fyers_model_create(client_id, access_token, false, NULL,
                               FYERS_LOG_INFO);
    fclose(file_read_ptr);
  }

  return model;
}
