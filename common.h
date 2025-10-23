/*
 * Configurations
 */
#define MAX_CONNECTIONS 10 /* Maximum number of connections that can be served at a time*/
#define LOG_FILE_PATH "log.txt" /* Server log file name */
#define REMOTE_FILE_DIR "./remote" /* Subdirectory on the server for user files */
#define CHUNK_SIZE 1024 /* 1KB per chunk */

/*
 * Server responses
 */
#define MESSAGE_RECEIVED_RESP "Message received"

/*
 * Common types
 */
struct FileInfoType
    {
    char fileName[128];
    unsigned int sizeInBytes;
    };

