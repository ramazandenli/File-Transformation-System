#include <json-c/json.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemastypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * This function prints out the correct usage of the program,
 * including various options for different operations.
 *
 * @param program_name The name of the program.
 */
void printUsage(char *program_name) {
    printf(
        "Usage: %s <input_file> <output_file> <type>\n"
        "       %s <xml_file> <xsd_file> <type>\n"
        "       %s <input_file> <type>\n"
        "       %s help\n"
        "Type 1: CSV to BIN\n"
        "Type 2: BIN to XML\n"
        "Type 3: XML validation with XSD\n",
        program_name, program_name, program_name, program_name);
}

/**
 * This struct represents a record in the CSV file.
 */
struct Record {
    char name[20];
    char surname[30];
    char stuId[11];
    char gender;
    char email[128];
    char phone[18];
    char letter_grade[3];
    int midterm;
    int project;
    int final;
    char regularStudent[1];
    int course_surveyRating;
};
typedef struct Record Record;

// Constants
char *setupParamsFileName = "setupParams.json";

/**
 * Converts a hexadecimal string to a decimal number.
 *
 * @param hex The hexadecimal string to convert.
 * @return The decimal number represented by the hexadecimal string.
 */
int hexToDecimal(char hex[]) {
    int len = strlen(hex);
    int decimal = 0;
    int base = 1; // Initialize base value to 16^0

    for (int i = len - 1; i >= 0; i--) {
        // If character is between 0-9, then convert it to decimal by subtracting 48 ('0' ASCII value)
        if (hex[i] >= '0' && hex[i] <= '9') {
            decimal += (hex[i] - '0') * base;
        }
        // If character is between A-F, then convert it to decimal by subtracting 55 ('A' ASCII value)
        else if (hex[i] >= 'A' && hex[i] <= 'F') {
            decimal += (hex[i] - 'A' + 10) * base;
        }
        // If character is between a-f, then convert it to decimal by subtracting 87 ('a' ASCII value)
        else if (hex[i] >= 'a' && hex[i] <= 'f') {
            decimal += (hex[i] - 'a' + 10) * base;
        }
        // Invalid character in hex string
        else {
            printf("Invalid hexadecimal string!\n");
            return -1;
        }
        base *= 16; // Update base value to 16^(position)
    }

    return decimal;
}

/**
 * Returns a pointer to a byte array of the given struct Record object,
 * starting from the keyStart index and ending at the keyEnd index.
 *
 * @param record The struct Record object to convert to a byte array.
 * @param keyStart The index at which to start the byte array.
 * @param keyEnd The index at which to end the byte array.
 * @return A pointer to the byte array.
 */
unsigned char *getBytes(Record record, int keyStart, int keyEnd) {
    unsigned int length = keyEnd - keyStart + 1;
    unsigned char *byteArray = malloc(length);

    memcpy(byteArray, ((unsigned char *)&record) + keyStart, length); // Copy the desired bytes to the byte array

    return byteArray;
}

/**
 *
 * This function sorts an array of records using the insertion sort algorithm.
 *
 * @param arr An array of records to be sorted.
 * @param n The number of elements in the array.
 * @param keyStart The starting index of the key within each record.
 * @param keyEnd The ending index of the key within each record.
 * @param isAsc Flag indicating the sorting order (0 for ascending, non-zero for descending).
 */
void insertionSort(Record arr[], int n, int keyStart, int keyEnd, int isAsc) {
    int i, key, j;
    for (i = 1; i < n; i++) {
        Record key = arr[i];
        j = i - 1;

        while (j >= 0 && (isAsc == 0 ? (strcmp(getBytes(arr[j], keyStart, keyEnd), getBytes(key, keyStart, keyEnd)) < 0) : (strcmp(getBytes(arr[j], keyStart, keyEnd), getBytes(key, keyStart, keyEnd)) > 0))) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}

/**
 * Convert a decimal number to big endian format.
 *
 * @param result - the buffer to store the result
 * @param num - the decimal number to convert
 */
void decimalToBigEndian(char *result, int num) {
    uint8_t *ptr = (uint8_t *)&num;

    for (int i = 0; i < sizeof(num); i++) {
        sprintf(&result[i * 2], "%02X", *(ptr + sizeof(num) - 1 - i));
    }
}

/**
 * Convert a decimal number to little endian format.
 *
 * @param result - the buffer to store the result
 * @param num - the decimal number to convert
 */
void decimalToLittleEndian(char *result, int num) {
    uint8_t *ptr = (uint8_t *)&num;

    for (int i = 0; i < sizeof(num); i++) {
        sprintf(&result[i * 2], "%02X", *(ptr + i));
    }
}

/**
 * Parses the setupParams JSON file and returns the corresponding values.
 *
 * @param jsonFileName The name of the JSON file.
 * @param dataFileNameP The pointer to the data file name.
 * @param keyStartP The pointer to the key start index.
 * @param keyEndP The pointer to the key end index.
 * @param orderP The pointer to the sorting order.
 * @return 0 on success, 1 on failure.
 **/
int parseSetupParams(char *jsonFileName, char *dataFileNameP, int *keyStartP, int *keyEndP, char *orderP) {
    // Declaration
    FILE *jsonFile;
    char buffer[1024];
    struct json_object *parsed_json;
    struct json_object *dataFileName;
    struct json_object *keyStart;
    struct json_object *keyEnd;
    struct json_object *order;

    // Check if the JSON file exists
    if (access(jsonFileName, 0) == -1) {
        printf("Error: %s does not exist\n", jsonFileName);
        return 1;
    }

    // Open the JSON file
    jsonFile = fopen(jsonFileName, "r");

    // Check if the JSON file was opened successfully
    if (jsonFile == NULL) {
        printf("Error opening params file: %s\n", jsonFileName);
        return 1;
    }

    // Read the contents of the JSON file
    fread(buffer, 1024, 1, jsonFile);
    fclose(jsonFile);

    // Parse the JSON file
    parsed_json = json_tokener_parse(buffer);

    // Check if the JSON was parsed successfully
    if (parsed_json == NULL) {
        printf("Error parsing params file: %s\n", jsonFileName);
        return 1;
    }

    // Get the values of each parameter
    json_object_object_get_ex(parsed_json, "dataFileName", &dataFileName);
    json_object_object_get_ex(parsed_json, "keyStart", &keyStart);
    json_object_object_get_ex(parsed_json, "keyEnd", &keyEnd);
    json_object_object_get_ex(parsed_json, "order", &order);

    // Check if the required parameters are present
    if (dataFileName == NULL || keyStart == NULL || keyEnd == NULL || order == NULL) {
        printf("Error: missing parameter '%s' in params file: %s\n", dataFileName == NULL ? "dataFileName" : keyStart == NULL ? "keyStart"
                                                                                                         : keyEnd == NULL     ? "keyEnd"
                                                                                                                              : "order",
               jsonFileName);
        return 1;
    }

    // Assign the values to the corresponding pointers
    strcpy(dataFileNameP, json_object_get_string(dataFileName));
    *keyStartP = json_object_get_int(keyStart);
    *keyEndP = json_object_get_int(keyEnd);
    strcpy(orderP, json_object_get_string(order));

    return 0;
}

/**
 * Writes a record to an XML file.
 *
 * @param record The record to be written.
 * @param id The ID of the record.
 * @param root_node The root node of the XML document.
 * @return 0 on success, 1 on failure.
 */
int xmlRecordWriter(Record record, int id, xmlNodePtr root_node) {
    // Declaration
    xmlNodePtr node = NULL, node1 = NULL;
    char buff[256];

    // Create row
    node = xmlNewChild(root_node, NULL, BAD_CAST "row", NULL);
    sprintf(buff, "%d", id);
    xmlNewProp(node, BAD_CAST "id", BAD_CAST buff);

    // Create student_info
    node1 = xmlNewChild(node, NULL, BAD_CAST "student_info", NULL);
    strcmp(record.name, "") == 0 ? NULL : xmlNewChild(node1, NULL, BAD_CAST "name", BAD_CAST record.name);
    strcmp(record.surname, "") == 0 ? NULL : xmlNewChild(node1, NULL, BAD_CAST "surname", BAD_CAST record.surname);
    strcmp(record.stuId, "") == 0 ? NULL : xmlNewChild(node1, NULL, BAD_CAST "stuID", BAD_CAST record.stuId);
    sprintf(buff, "%c", record.gender);
    strcmp(buff, "") == 0 ? NULL : xmlNewChild(node1, NULL, BAD_CAST "gender", BAD_CAST buff);
    strcmp(record.email, "") == 0 ? NULL : xmlNewChild(node1, NULL, BAD_CAST "email", BAD_CAST record.email);
    strcmp(record.phone, "") == 0 ? NULL : xmlNewChild(node1, NULL, BAD_CAST "phone", BAD_CAST record.phone);

    // Create grade_info
    node1 = xmlNewChild(node, NULL, BAD_CAST "grade_info", NULL);
    strcmp(record.letter_grade, "") == 0 ? NULL : xmlNewProp(node1, BAD_CAST "letter_grade", BAD_CAST record.letter_grade);

    sprintf(buff, "%d", record.midterm);
    strcmp(buff, "") == 0 ? NULL : xmlNewChild(node1, NULL, BAD_CAST "midterm", BAD_CAST buff);
    sprintf(buff, "%d", record.project);
    strcmp(buff, "") == 0 ? NULL : xmlNewChild(node1, NULL, BAD_CAST "project", BAD_CAST buff);
    sprintf(buff, "%d", record.final);
    strcmp(buff, "") == 0 ? NULL : xmlNewChild(node1, NULL, BAD_CAST "final", BAD_CAST buff);

    // Create regularStudent
    strcmp(record.regularStudent, "") == 0 ? NULL : xmlNewChild(node, NULL, BAD_CAST "regularStudent", BAD_CAST(strstr(record.regularStudent, "\xF0\x9F\x91\x8D") != NULL ? "\xF0\x9F\x91\x8D" : (strstr(record.regularStudent, "\xF0\x9F\x91\x8E") != NULL ? "\xF0\x9F\x91\x8E" : "")));

    // Create course_surveyRating
    sprintf(buff, "%d", record.course_surveyRating);
    node1 = (strcmp(buff, "") == 0 || strcmp(buff, "0") == 0) ? NULL : xmlNewChild(node, NULL, BAD_CAST "course_surveyRating", BAD_CAST buff);
    if (node1 != NULL) {
        decimalToBigEndian(buff, record.course_surveyRating);
        xmlNewProp(node1, BAD_CAST "hexVal_bigEnd", BAD_CAST buff);

        decimalToLittleEndian(buff, record.course_surveyRating);
        xmlNewProp(node1, BAD_CAST "hexVal_littleEnd", BAD_CAST buff);

        sprintf(buff, "%d", hexToDecimal(buff));
        xmlNewProp(node1, BAD_CAST "decimal", BAD_CAST buff);
    }

    return 0;
}

/**
 * Type 1: CSV to BIN
 *
 * @param inputFileName The name of the CSV file.
 * @param outputFileName The name of the binary file.
 * @return 0 on success, 1 on failure.
 */
int type1(char *inputFileName, char *outputFileName) {
    // Check if the CSV file exists
    if (access(inputFileName, 0) == -1) {
        printf("Error: %s does not exist\n", inputFileName);
        return 1;
    }

    // Open the CSV and binary files
    FILE *csvFile = fopen(inputFileName, "r");
    FILE *binaryFile = fopen(outputFileName, "wb");

    // Check if the files were opened successfully
    if (csvFile == NULL) {
        printf("Error opening %s\n", inputFileName);
        return 1;
    } else if (binaryFile == NULL) {
        printf("Error opening %s\n", outputFileName);
        return 1;
    }

    // Declaration
    char line[350];

    // Skip the first line of the CSV file
    fgets(line, sizeof(line), csvFile);

    // Read each line of the CSV file
    while (fgets(line, sizeof(line), csvFile)) {
        Record record;
        char *token = NULL;
        char *line_copy = strdup(line); // Duplicate line to preserve original
        char *temp = line_copy;         // Pointer to traverse the duplicate string

        // Tokenize the line
        for (int i = 0; i < 12; i++) {
            // Skip empty fields
            if ((token = strsep(&temp, ",")) == NULL) {
                continue;
            }

            // Assign the token to the corresponding field
            switch (i) {
            case 0:
                strcpy(record.name, token);
                break;
            case 1:
                strcpy(record.surname, token);
                break;
            case 2:
                strcpy(record.stuId, token);
                break;
            case 3:
                record.gender = token[0];
                break;
            case 4:
                strcpy(record.email, token);
                break;
            case 5:
                strcpy(record.phone, token);
                break;
            case 6:
                strcpy(record.letter_grade, token);
                break;
            case 7:
                record.midterm = atoi(token);
                break;
            case 8:
                record.project = atoi(token);
                break;
            case 9:
                record.final = atoi(token);
                break;
            case 10:
                strcpy(record.regularStudent, token);
                break;
            case 11:
                record.course_surveyRating = atoi(token);
                break;
            default:
                break;
            }
        }

        free(line_copy);                                // Free the duplicated string after usage
        fwrite(&record, sizeof(Record), 1, binaryFile); // Write the record to the binary file
    }

    // Close the files
    fclose(csvFile);
    fclose(binaryFile);

    // Print success message
    printf("Binary file created: %s\n", outputFileName);

    return 0;
}

/**
 * Type 2: BIN to XML
 *
 * @param xmlFileName The name of the XML file.
 * @return 0 on success, 1 on failure.
 */
int type2(char *xmlFileName) {
    // Declaration
    char dataFileName[256];
    int keyStart;
    int keyEnd;
    char order[5];

    // Parse the setup parameters
    if (parseSetupParams(setupParamsFileName, dataFileName, &keyStart, &keyEnd, order) != 0) {
        return 1;
    }

    // Check if the binary file exists
    if (access(dataFileName, 0) == -1) {
        printf("Error: %s does not exist\n", dataFileName);
        return 1;
    }

    // Declaration
    FILE *binaryFile = fopen(dataFileName, "rb");
    Record record;
    Record *records = NULL;

    // Check if the binary file was opened successfully
    if (binaryFile == NULL) {
        printf("Error opening %s\n", dataFileName);
        return 1;
    }

    // Read the records from the binary file
    int size = 0; // Initial array size
    while (fread(&record, sizeof(Record), 1, binaryFile)) {
        if (record.stuId == NULL) { // End of file
            break;
        }
        records = realloc(records, (size + 1) * sizeof(Record)); // Resize the array
        records[size] = record;                                  // Add the record
        size++;                                                  // Increment the array size
    }

    insertionSort(records, size, keyStart - 1 >= 0 ? keyStart - 1 : 0, keyEnd, strcmp(order, "ASC") == 0); // Sort the records

    // XML pointers
    xmlDocPtr doc = NULL;
    xmlNodePtr root_node = NULL;

    // Create the XML doc and root node
    doc = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, BAD_CAST "records");
    xmlDocSetRootElement(doc, root_node);

    // Write the records to the XML file
    for (int i = 0; i < size; i++) {
        xmlRecordWriter(records[i], i + 1, root_node);
    }

    xmlSaveFormatFileEnc(xmlFileName, doc, "UTF-8", 1); // Save the XML file

    xmlFreeDoc(doc); // Free the document

    xmlCleanupParser(); // Free the global variables that may have been allocated by the parser.

    // Print success message
    printf("XML file created: %s\n", xmlFileName);

    return 0;
}

/**
 * Type 3: XML validation with XSD
 *
 * @param xmlFileName The name of the XML file.
 * @param xsdFileName The name of the XSD file.
 * @return 0 on success, 1 on failure.
 */
int type3(char *xmlFileName, char *xsdFileName) {
    // Check if the XML and XSD files exist
    if (access(xmlFileName, 0) == -1) {
        printf("Error: %s does not exist\n", xmlFileName);
        return 1;
    }
    if (access(xsdFileName, 0) == -1) {
        printf("Error: %s does not exist\n", xsdFileName);
        return 1;
    }

    printf("Validating %s with %s\n", xmlFileName, xsdFileName);

    xmlDocPtr doc;
    xmlSchemaPtr schema = NULL;
    xmlSchemaParserCtxtPtr ctxt;

    xmlLineNumbersDefault(1);                   // set line numbers, 0> no substitution, 1>substitution
    ctxt = xmlSchemaNewParserCtxt(xsdFileName); // create an xml schemas parse context
    schema = xmlSchemaParse(ctxt);              // parse a schema definition resource and build an internal XML schema
    xmlSchemaFreeParserCtxt(ctxt);              // free the resources associated to the schema parser context

    doc = xmlReadFile(xmlFileName, NULL, 0); // parse an XML file
    if (doc == NULL) {
        fprintf(stderr, "Could not parse %s\n", xmlFileName);
    } else {
        xmlSchemaValidCtxtPtr ctxt; // structure xmlSchemaValidCtxt, not public by API
        int ret;

        ctxt = xmlSchemaNewValidCtxt(schema);  // create an xml schemas validation context
        ret = xmlSchemaValidateDoc(ctxt, doc); // validate a document tree in memory
        if (ret == 0) {                        // validated
            printf("Validation successful\n");
        } else if (ret > 0) { // positive error code number
            printf("Validation failed\n");
        } else { // internal or API error
            printf("%s validation generated an internal error\n", xmlFileName);
        }
        xmlSchemaFreeValidCtxt(ctxt); // free the resources associated to the schema validation context
        xmlFreeDoc(doc);
    }
    // free the resource
    if (schema != NULL)
        xmlSchemaFree(schema); // deallocate a schema structure

    xmlSchemaCleanupTypes(); // cleanup the default xml schemas types library
    xmlCleanupParser();      // cleans memory allocated by the library itself
    xmlMemoryDump();         // memory dump

    return 0;
}

/*
Test:
gcc 2021510063_2022510111_2022510159.c -o converter -I/usr/include/libxml2 -lxml2 -ljson-c
./converter records.csv records.dat 1
./converter records.xml 2
./converter records.xml 2021510063_2022510111_2022510159.xsd 3
*/

int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 4) {
        if (argc == 1 || (argc == 2 && strcmp(argv[1], "help")) == 0) {
            printUsage(argv[0]);
            return 0;
        }
        printf("Error: Invalid number of arguments\n");
        printUsage(argv[0]);
        return 1;
    }

    int ret = 0;

    if (argc == 4 && strcmp(argv[3], "1") == 0) {
        ret = type1(argv[1], argv[2]);
    } else if (argc == 3 && strcmp(argv[2], "2") == 0) {
        ret = type2(argv[1]);
    } else if (argc == 4 && strcmp(argv[3], "3") == 0) {
        ret = type3(argv[1], argv[2]);
    } else {
        printf("Error: Invalid arguments\n");
        printUsage(argv[0]);
        return 1;
    }

    return ret;
}
