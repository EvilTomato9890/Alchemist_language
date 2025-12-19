#include "text_utils.h"

Text* TextCtor(const char* file_name) {
    my_assert(file_name, NULLPTR, NULL);

    Text* text = (Text*)calloc(1, sizeof(Text));
    my_assert(text, NULLPTR, NULL);

    text->file_name = file_name;
    
    CodeError_t error_code = ReadFile(text);

    my_assert(error_code == NOTHING, error_code, NULL);

    return text;
}

int GetFileSize(const char* file_name) {
    struct stat file_stat; 
    int stat_result = stat(file_name, &file_stat);

    my_assert(stat_result != -1, FILE_ERR, -1);

    return file_stat.st_size;
}

CodeError_t ReadFile(Text* text) {
    my_assert(text, NULLPTR, NULLPTR);

    text->file_size = GetFileSize(text->file_name);
    my_assert(text->file_size != -1, FILE_ERR, FILE_ERR);

    text->buf = (char*)calloc(text->file_size, sizeof(char));
    my_assert(text->buf, NULLPTR, NULLPTR);

    FILE* fp = fopen(text->file_name, "rb");
    my_assert(fp, FILE_ERR, FILE_ERR);

    int read_symbols = fread(text->buf, sizeof(char), text->file_size, fp);
    my_assert(read_symbols == text->file_size, FILE_ERR, FILE_ERR);

    int close_check = fclose(fp);
    my_assert(!close_check, FILE_ERR, FILE_ERR);

    return NOTHING;
}