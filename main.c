#include "./fyers-session-helpers.c"
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/typecheck-gcc.h>
#include <fyers_api.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

unsigned int show_choice(void);

// size_t write_data(char *buffer, size_t size, size_t nmemb, void *userp);
int download_symbols_master(void);

void parse_symbols_json(cJSON *instrument, FILE *writePtr);
int create_instruments_master_file(void);

void show_next_3_futures(fyers_model_t *model, const char *input_symbol);
void find_cotango(fyers_model_t *model);

struct instr {
  char *symbol;
  int exchange;
  int exchange_token;
  int type;
  double bid;
  double ask;
  double ltp;
  bool has_futures;
};
typedef struct instr INSTRUMENT;

int main(void) {
  // Init libcurl
  curl_global_init(CURL_GLOBAL_ALL);

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
  unsigned int choice;
  while ((choice = show_choice()) != 5) {
    switch (choice) {
    case 1:
      download_symbols_master();
      create_instruments_master_file();
      break;
    case 2:
      puts("Enter your symbol");
      scanf("%s", input_symbol);
      show_next_3_futures(model, input_symbol);
      break;
    case 3:
      find_cotango(model);
      break;
    default:
      puts("NOT_IMPLEMENTED");
      break;
    }
  }

  fyers_model_destroy(model);
  fyers_session_destroy(session);
  fyers_cleanup();
  curl_global_cleanup();

  return 0;
}

unsigned int show_choice(void) {
  puts("");
  unsigned int choiceTaken;
  printf("%s", "Select Your Choice\n"
               "1. SEED the db\n"
               "2. CHECK on an EQ\n"
               "3. RUN cotango across all EQs\n"
               "5. STOP this program\n"
               "> ");
  scanf("%u", &choiceTaken);
  return choiceTaken;
}

// size_t write_data(char *buffer, size_t size, size_t nmemb, void *userp) {
//   printf("%s", buffer);
//   puts("");
//   return size;
// }

int download_symbols_master(void) {
  CURL *handle = curl_easy_init();
  curl_easy_setopt(
      handle, CURLOPT_URL,
      "https://public.fyers.in/sym_details/NSE_CM_sym_master.json");
  FILE *cfPtr;
  if ((cfPtr = fopen("./data/NSE_CM_sym_master.json", "w")) == NULL) {
    fprintf(stderr, "Failed to create file for sym master");
    return 1;
  } else {
    // curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, cfPtr);
    puts("file transferred");

    curl_easy_perform(handle);
    fclose(cfPtr);
  }
  curl_easy_cleanup(handle);
  return 0;
}

void parse_symbols_json(cJSON *instrument, FILE *writePtr) {
  cJSON *exchange_raw =
      cJSON_GetObjectItemCaseSensitive(instrument, "exchange");
  cJSON *token_raw = cJSON_GetObjectItemCaseSensitive(instrument, "exToken");
  cJSON *type_raw = cJSON_GetObjectItemCaseSensitive(instrument, "exInstType");
  cJSON *has_futures_raw =
      cJSON_GetObjectItemCaseSensitive(instrument, "has_futures");

  if (!cJSON_IsNumber(exchange_raw) || !cJSON_IsNumber(token_raw) ||
      !cJSON_IsNumber(type_raw) || !cJSON_IsBool(has_futures_raw)) {
    fprintf(stderr, "Typing mismatch while JSON parsing instrument master\n");
  } else {
    char *instrumentId = instrument->string;
    INSTRUMENT currentInstr = {instrumentId,
                               exchange_raw->valueint,
                               token_raw->valueint,
                               type_raw->valueint,
                               0.0,
                               0.0,
                               0.0,
                               has_futures_raw->valueint};
    fseek(writePtr, currentInstr.exchange_token * sizeof(INSTRUMENT), SEEK_SET);
    fwrite(&currentInstr, sizeof(INSTRUMENT), 1, writePtr);
  }
}

int create_instruments_master_file(void) {
  FILE *cfPtr;
  if ((cfPtr = fopen("./data/NSE_CM_sym_master.json", "r")) == NULL) {
    fprintf(stderr, "Failed to create seed file for instrument master");
    return 1;
  }
  // clock_t begin = clock();
  fseek(cfPtr, 0, SEEK_END);
  long length = ftell(cfPtr);
  fseek(cfPtr, 0, SEEK_SET);

  char *content = (char *)malloc(length + 1);
  fread(content, 1, length, cfPtr);
  fclose(cfPtr);

  content[length] = '\0';

  cJSON *root = cJSON_ParseWithLength(content, length);
  if (!root) {
    fprintf(stderr, "Invalid JSON root");
    free(content);
    return 2;
  }

  FILE *writePtr;
  if ((writePtr = fopen("./db/instruments.dat", "wb+")) == NULL) {
    fprintf(stderr, "Failed to create instruments db");
    return 1;
  }
  cJSON *instrument;
  unsigned int index = 0;
  cJSON_ArrayForEach(instrument, root) {
    parse_symbols_json(instrument, writePtr);
    index++;
  }
  // clock_t end = clock();
  // printf("Time Spent: %.2f\n", (double)(end - begin)/CLOCKS_PER_SEC);
  printf("Wrote %u instruments", index);

  cJSON_Delete(root);
  free(content);
  fclose(writePtr);

  return 0;
}

void show_next_3_futures(fyers_model_t *model, const char *input_symbol) {
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

  int index = 0;
  int last_prices[4] = { 0 }; // Expected at max 3 FUTs
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

    if (index == 0) {
      printf("| %s \t\t| %.2f \t| %.2f \t\t| %.2f \t\t| %d \t| %s |\t\n",
             symbol, lp, ch, chp, volume, expiry);
    } else {
      printf("| %s \t| %.2f \t| %.2f \t\t| %.2f \t\t| %d \t| %s |\t\n", symbol,
             lp, ch, chp, volume, expiry);
      last_prices[index - 1] = lp;
    }
    index++;
  }

  puts("");
  for (int i=1; i<4; i++) {
    if (last_prices[i] == 0) continue;
    double ch_pct = (last_prices[i]-last_prices[i-1])*100.0/(1.00*last_prices[i]);
    printf("FUT change percentage: %0.2f\n", ch_pct);
  }

  cJSON_Delete(futures_json);
  fyers_response_destroy(future_chain);
}

void find_cotango(fyers_model_t *model) { puts("not yet"); }
