void vera_spiffs_list_dir(fs::FS &fs, const char *dirname, uint8_t levels);
void vera_spiffs_read_file(fs::FS &fs, const char *path);
void vera_spiffs_write_file(fs::FS &fs, const char *path, const char *message);
void vera_spiffs_append_file(fs::FS &fs, const char *path, const char *message);
void vera_spiffs_rename_file(fs::FS &fs, const char *path1, const char *path2);
void vera_spiffs_delete_file(fs::FS &fs, const char *path);
void vera_spiffs_test_file_io(fs::FS &fs, const char *path);
