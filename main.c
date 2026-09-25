#include "./fyers-session-helpers.c"
#include <cjson/cJSON.h>
#include <fyers_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void show_next_3_futures(fyers_model_t *model, const char* input_symbol);

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
  puts("Fyers is connected");

  char input_symbol[63] = {0};
  puts("Enter your symbol");
  scanf("%s", input_symbol);
  show_next_3_futures(model, input_symbol);

  fyers_model_destroy(model);
  fyers_session_destroy(session);
  fyers_cleanup();

  return 0;
}

void show_next_3_futures(fyers_model_t *model, const char* input_symbol) {
  fyers_response_t *future_chain =
      fyers_model_get_futures_chain(model, input_symbol);
  cJSON *futures_json = cJSON_Parse(future_chain->data);
  if (futures_json == NULL) {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL) {
      fprintf(stderr, "Error: %s\n", error_ptr);
    }
    cJSON_Delete(futures_json);
    fyers_response_destroy(future_chain);
    return;
  }
  const cJSON *future_arr =
      cJSON_GetObjectItemCaseSensitive(futures_json, "data");
  const cJSON *future = NULL;
  puts("| Symbol \t\t| LastPrice \t| Change \t| Change %age \t| Volume \t| "
       "Expiry \t|");

  cJSON_ArrayForEach(future, future_arr) {
    const cJSON *ch_raw = cJSON_GetObjectItemCaseSensitive(future, "ch");
    const cJSON *chp_raw = cJSON_GetObjectItemCaseSensitive(future, "chp");
    const cJSON *expiry_raw =
        cJSON_GetObjectItemCaseSensitive(future, "expiry");
    const cJSON *lp_raw = cJSON_GetObjectItemCaseSensitive(future, "lp");
    const cJSON *symbol_raw =
        cJSON_GetObjectItemCaseSensitive(future, "symbol");
    const cJSON *volume_raw =
        cJSON_GetObjectItemCaseSensitive(future, "volume");

    if (!cJSON_IsNumber(ch_raw) || !cJSON_IsNumber((chp_raw)) ||
        !cJSON_IsString(expiry_raw) || !cJSON_IsNumber(lp_raw) ||
        !cJSON_IsString(symbol_raw) || !cJSON_IsNumber(volume_raw)) {
      fprintf(stderr, "Failed to get data");

      cJSON_Delete(futures_json);
      fyers_response_destroy(future_chain);
      return;
    }

    double ch = ch_raw->valuedouble;
    double chp = chp_raw->valuedouble;
    double lp = lp_raw->valuedouble;
    const char *symbol = symbol_raw->valuestring;
    int volume = volume_raw->valueint;

    time_t expiry_int = atoi(expiry_raw->valuestring);
    struct tm ts;
    char expiry[63];
    ts = *localtime(&expiry_int);
    strftime(expiry, sizeof(expiry), "%d-%m %Y", &ts);

    printf("| %s \t| %.2f \t| %.2f \t\t| %.2f \t\t| %d \t| %s |\t\n", symbol,
           lp, ch, chp, volume, expiry);
  }

  puts("");

  cJSON_Delete(futures_json);
  fyers_response_destroy(future_chain);
}
