#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// support for mkdir, access, F_OK
#include <sys/stat.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 1024
#define MAX_HEADER_COLS 50
#define INITIAL_ROW_CAPACITY 10

#define MAXNBFILE 1000

// A struct to hold all our parsed CSV data
typedef struct {
    char *headers[MAX_HEADER_COLS];
    char ***data; // A 2D array of strings: data[row][col]
    int num_rows;
    int row_capacity;
    int num_cols;
} CsvData;


// A stuct to hold files info relevant to PDI
typedef struct {
    char FRAMEID[100];
    float MJD;
    float RETANG1;
    float RETPOS1;
    int U_CAMERA;
    float TINT;
    int NDIT;
} FileInfo;

// array of FileInfo entries
int nbfile = 0;
FileInfo fileinf[MAXNBFILE];




// load files from remote using rsync
int load_from_remote(const char *filename, const char *remotedir) {
    char command[1024];

    // prepare rsync command
    sprintf(command, "rsync -au --progress %s/%s* ./data/", remotedir, filename);

    // execute rsync command
    if (system(command) != 0) {
        printf("Downloaded files %s\n", filename);
    } else {
        printf("Error downloading files %s\n", filename);
        return 1;
    }

    return 0;
}



// Function to free all dynamically allocated memory
void free_csv(CsvData *csv) {
    if (!csv) return;

    // Free headers
    for (int i = 0; i < csv->num_cols; i++) {
        free(csv->headers[i]);
    }

    // Free data cells
    if (csv->data) {
        for (int i = 0; i < csv->num_rows; i++) {
            if (csv->data[i]) {
                for (int j = 0; j < csv->num_cols; j++) {
                    free(csv->data[i][j]);
                }
            } // No need for else, malloc would have been checked
            free(csv->data[i]);
        }
        free(csv->data);
    }
}

// Function to load and print CSV file
void load_and_print_csv(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    CsvData csv = { .data = NULL, .num_rows = 0, .row_capacity = 0, .num_cols = 0 };
    char line[MAX_LINE_LENGTH];


    int FRAMEIDtokeni = -1;


    // 1. READ AND PARSE HEADER
    // -------------------------
    if (fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "Error: Could not read header or file is empty.\n");
        fclose(file);
        return;
    }
    line[strcspn(line, "\n")] = 0; // Remove newline

    char *header_copy = strdup(line); // Use a copy for strtok
    if (!header_copy) {
        fprintf(stderr, "Memory allocation failed for header copy.\n");
        fclose(file);
        return;
    }

    char *token = strtok(header_copy, ",");
    while (token != NULL && csv.num_cols < MAX_HEADER_COLS) {
        csv.headers[csv.num_cols] = strdup(token); // Allocate and copy token
        if (!csv.headers[csv.num_cols]) {
            fprintf(stderr, "Memory allocation failed for header token.\n");
            free(header_copy);
            free_csv(&csv);
            fclose(file);
            return;
        }
        if (strcmp(token, "FRAMEID") == 0) {
            FRAMEIDtokeni = csv.num_cols;
        }
        csv.num_cols++;
        token = strtok(NULL, ",");
    }
    free(header_copy); // Clean up the copy

    // 2. READ DATA ROWS DYNAMICALLY
    // -----------------------------
    csv.row_capacity = INITIAL_ROW_CAPACITY;
    csv.data = malloc(csv.row_capacity * sizeof(char**));
    if (csv.data == NULL) {
        fprintf(stderr, "Error: Initial memory allocation failed for rows.\n");
        free_csv(&csv);
        fclose(file);
        return;
    }

    int current_row = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = 0;

        // Grow the array of rows if capacity is reached
        if (current_row >= csv.row_capacity) {
            csv.row_capacity *= 2;
            char ***temp_data = realloc(csv.data, csv.row_capacity * sizeof(char**));
            if (temp_data == NULL) {
                fprintf(stderr, "Error: Memory reallocation failed for rows.\n");
                csv.num_rows = current_row;
                free_csv(&csv);
                fclose(file);
                return;
            }
            csv.data = temp_data;
        }

        csv.data[current_row] = malloc(csv.num_cols * sizeof(char*));
        if (csv.data[current_row] == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for columns.\n");
            csv.num_rows = current_row; // Set correct row count before freeing
            free_csv(&csv);
            fclose(file);
            return;
        }


        // Parse the line and fill the current row
        token = strtok(line, ",");
        for (int i = 0; i < csv.num_cols; i++) {
            if (token != NULL) {
                csv.data[current_row][i] = strdup(token);
                if (csv.data[current_row][i] == NULL) {
                    fprintf(stderr, "Memory allocation failed for data cell.\n");
                    // Free the already allocated cells in the current row
                    for (int j = 0; j < i; j++) {
                        free(csv.data[current_row][j]);
                    }
                    free(csv.data[current_row]);
                    // Set num_rows to only the successfully completed rows
                    csv.num_rows = current_row;
                    // Free the rest of the structure
                    free_csv(&csv);
                    fclose(file);
                    return;
                }
                else
                {
                    if (i == FRAMEIDtokeni)
                    {
                        strcpy(fileinf[nbfile].FRAMEID, token);
                    }
                }
                token = strtok(NULL, ",");
            } else {
                // Handle missing values by storing an empty string
                csv.data[current_row][i] = strdup("");
                if (csv.data[current_row][i] == NULL) {
                    // Omitting full cleanup for brevity, but it would be the same as above
                    fprintf(stderr, "Memory allocation failed for empty data cell.\n");
                    csv.num_rows = current_row;
                    free_csv(&csv);
                    return;
                }
            }
        }
        current_row++;
        nbfile++;
    }
    csv.num_rows = current_row;
    fclose(file);


    // 3. PRINT RESULTS AND CLEAN UP
    // -----------------------------
    printf("✅ CSV file loaded successfully!\n\n");
    printf("--- HEADER INFO ---\n");
    printf("Number of columns: %d\n", csv.num_cols);
    for (int i = 0; i < csv.num_cols; i++) {
        printf("Column %d: %s\n", i + 1, csv.headers[i]);
    }

    printf("\n--- FILE DATA ---\n");
    for (int i = 0; i < csv.num_rows; i++) {
        for (int j = 0; j < csv.num_cols; j++) {
            printf("%-15s", csv.data[i][j]);
            if (j < csv.num_cols - 1) printf("| ");
        }
        printf("\n");
    }

    // Free all allocated memory
    free_csv(&csv);
    printf("\n✅ Memory freed successfully.\n");
}




int main(int argc, char *argv[]) {
    // We now expect 3 arguments: the program name, the CSV file, and remote directory.
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <csv_file> <remotedir>\n", argv[0]);
        return 1;
    }
    const char *filename = argv[1];
    const char *remotedir = argv[2];

    printf("File to load: %s\n", filename);
    printf("Remote directory: %s\n", remotedir);


    load_and_print_csv(filename);

    // create ./data directory if it does not exist
    if (access("./data", F_OK) != 0) {
        if (mkdir("./data", 0777) != 0) {
            perror("Error creating ./data directory");
            return 1;
        }
    }


    for(int filei = 0; filei < nbfile; filei++)
    {
        printf("FRAMEID: %s\n", fileinf[filei].FRAMEID);

        // skip if destination file "FRAMEID.txt" already exists

        // append ".txt" to FRAMEID
        char filename[100];
        strcpy(filename, fileinf[filei].FRAMEID);
        strcat(filename, ".txt");
        if (access(filename, F_OK) == 0) {
            load_from_remote(fileinf[filei].FRAMEID, remotedir);
            continue;
        }
        else
        {
            printf("    File already downloaded - skipping download\n");
        }
    };


    return 0;
}