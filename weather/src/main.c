#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <json-glib/json-glib.h>

#define REQUEST_BUF_SIZE 256

#define CUR_COND "current_condition"
#define CUR_COND_LEN 17

#define WEATHER "weather"
#define WEATHER_LEN 7

void print_weather(const char* city_name, const char* json_data) {
    JsonParser* parser = json_parser_new();
    GError* err = NULL;
    if (!json_parser_load_from_data(parser, json_data, -1, &err)) {
        fprintf(stderr, "error in parsing json data %s\n", err->message);
        printf("data:\n%s\n\n", json_data);
        g_error_free(err);
        g_object_unref(parser);
    }
    JsonNode* root_node = json_parser_get_root(parser);
    assert(root_node && "unable to parse root node");
    JsonObject* root_obj = json_node_get_object(root_node);

    GList* root_list = json_object_get_members(root_obj);
    printf("city: %s\n", city_name);
    for (; root_list != NULL; root_list = root_list->next) {
        if (0 == strncmp((const char*)root_list->data, CUR_COND, CUR_COND_LEN)) {
            JsonArray* cur_cond_arr = json_object_get_array_member(root_obj, (const char*)root_list->data);
            assert(cur_cond_arr);
            JsonNode* cur_cond_node = json_array_get_element(cur_cond_arr, 0);
            JsonObject* cur_cond_obj = json_node_get_object(cur_cond_node);

            JsonArray* weather_desc_arr = json_object_get_array_member(cur_cond_obj, "weatherDesc");
            JsonNode* weather_desc_node = json_array_get_element(weather_desc_arr, 0);
            JsonObject* weather_desc_obj = json_node_get_object(weather_desc_node);

            printf("\tweather       : %s\n\twind direction: %s\n\twind speed    : %s km/h\n",
                   json_object_get_string_member(weather_desc_obj, "value"),
                   json_object_get_string_member(cur_cond_obj, "winddir16Point"),
                   json_object_get_string_member(cur_cond_obj, "windspeedKmph"));
        } else if (0 == strncmp((const char*)root_list->data, WEATHER, WEATHER_LEN)) {
            JsonArray* weather_arr = json_object_get_array_member(root_obj, (const char*)root_list->data);
            assert(weather_arr);
            JsonNode* current_day_node = json_array_get_element(weather_arr, 0);
            JsonObject* current_day_obj = json_node_get_object(current_day_node);
            printf("\tmin temp      : %s \xE2\x84\x83\n\tmax temp      : %s \xE2\x84\x83\n",
                   json_object_get_string_member(current_day_obj, "mintempC"),
                   json_object_get_string_member(current_day_obj, "maxtempC"));
        }
    }
    g_list_free(root_list);
    g_object_unref(parser);
}

typedef struct {
    char* data;
    size_t size;
} response;

size_t response_cb(void* data, size_t size, size_t nmemb, void* user_data) {
    size_t real_size = size * nmemb;
    response* resp = (response*)user_data;

    char* ptr = realloc(resp->data, resp->size + real_size);
    if (!ptr) {
        perror("not enough memory to read response\n");
        return 0;
    }
    resp->data = ptr;
    memcpy(resp->data + resp->size - 1, data, real_size);
    resp->size += real_size;
    resp->data[resp->size] = '\0';
    return real_size;
}

void print_help() {
    printf("OPTIONS:\n"
           "\t-c, --city city weather you are interested in\n"
           "USAGE:\n"
           "\tweather [-c city]\n");
}

int main(int argc, char** argv) {
    int opt_char = 0;
    int opt_idx = 0;
    const char* city_name = NULL;
    struct option opts[] = {
        {"help", no_argument, 0, 'h'},
        {"city", required_argument, 0, 'c'},
        {0, 0, 0, 0},
    };

    while (-1 != (opt_char = getopt_long(argc, argv, ":hc:", opts, &opt_idx))) {
        switch (opt_char) {
            case 'h': {
                print_help();
                exit(EXIT_SUCCESS);
            }
            case 'c': {
                city_name = optarg;
                break;
            }
            case ':': {
                fprintf(stderr, "missing argument for '%c' option\n\n", optopt);
                print_help();
                exit(EXIT_FAILURE);
            }
            case '?': {
                fprintf(stderr, "unknown option: '%c'\n\n", optopt);
                print_help();
                exit(EXIT_FAILURE);
            }
        }
    }

    if (NULL == city_name) {
        fprintf(stderr, "missing option: '-c'\n");
        print_help();
        exit(EXIT_FAILURE);
    }

    CURL* curl_handle = curl_easy_init();
    if (NULL == curl_handle) {
        perror("unable to init curl");
        exit(EXIT_FAILURE);
    }

    response resp = {.data = malloc(1), .size = 1};
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, response_cb);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&resp);

    char request_buf[REQUEST_BUF_SIZE];
    snprintf(request_buf, REQUEST_BUF_SIZE, "https://wttr.in/%s?format=j1", city_name);
    curl_easy_setopt(curl_handle, CURLOPT_URL, request_buf);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1);

    CURLcode res = curl_easy_perform(curl_handle);
    if (CURLE_OK != res) {
        fprintf(stderr, "request failed: %s\n", curl_easy_strerror(res));
    }

    print_weather(city_name, resp.data);

    free(resp.data);
    curl_easy_cleanup(curl_handle);
    exit(EXIT_SUCCESS);
}
