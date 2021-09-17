#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "windows.h"
#include "time.h"
#include "md5.h"

#define KEYSIZE 16
#define MATRIX_DIMENSION 4
#define MATRIX_DIMENSION_64 64
#define Sbox_size 256   // 2^8 Tamaño de S-box

enum SIMPLE_OPERATION {SUB,ADD};

int numPlaces(int n) { // Esta es la version mas rapida de calcular los digitos de un entero
	if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
	if (n < 10) return 1;
	if (n < 100) return 2;
	if (n < 1000) return 3;
	if (n < 10000) return 4;
	if (n < 100000) return 5;
	if (n < 1000000) return 6;
	if (n < 10000000) return 7;
	if (n < 100000000) return 8;
	if (n < 1000000000) return 9;
	/*      2147483647 is 2^31-1 - add more ifs as needed
	   and adjust this final return as well. */
	return 10;
}

#pragma region Here are functions for input/output
#define NOOP ((void)0)
#define ENABLE_PRINTS 1					// Affects the PRINT() functions. If 0 does not print anything. If 1 traces are printed.
#define PRINT(...) do { if (ENABLE_PRINTS) printf(__VA_ARGS__); else NOOP;} while (0)
#define PRINT1(...) PRINT("    "); PRINT(__VA_ARGS__)
#define PRINT2(...) PRINT("        "); PRINT(__VA_ARGS__)
#define PRINT3(...) PRINT("            "); PRINT(__VA_ARGS__)
#define PRINT4(...) PRINT("                "); PRINT(__VA_ARGS__)
#define PRINT5(...) PRINT("                    "); PRINT(__VA_ARGS__)
#define PRINTX(DEPTH, ...) do { if (ENABLE_PRINTS) { for (int x=0; x<DEPTH; x++){ printf("    ");} printf(__VA_ARGS__); } else NOOP;} while (0)
#define PRINT_HEX(BUF, BUF_SIZE) print_hex(#BUF, BUF, BUF_SIZE);
#define PRINT_SMATRIX(BUF,BUF_SIZE) print_square_matrix(BUF, BUF_SIZE);

DWORD print_hex(char* buf_name, void* buf, int size) {
	if (ENABLE_PRINTS) {
		//printf("First %d bytes of %s contain:\n", size, buf_name);

		//byte [size*3 + size/32 + 1] str_fmt_buf;
		char* full_str = NULL;
		char* target_str = NULL;
		//int total = 0;

		// Size of string will consist on:
		//   (size*3)			 - 3 characters for every byte (2 hex characters plus 1 space). Space changed for '\n' every 32 bytes
		//   (size/8 - size/32)	 - Every 8 bytes another space is added after the space (if it is not multiple of 32, which already has '\n' instead)
		//   (1)				 - A '\n' is added at the end
		full_str = calloc((size * 3) + (size / 8 - size / 32) + (1), sizeof(char));
		if (full_str == NULL) {
			return ERROR_NOT_ENOUGH_MEMORY;
		}
		target_str = full_str;

		for (int i = 0; i < size; i++) {
			if ((i + 1) % 32 == 0) {
				target_str += sprintf(target_str, "%02hhX\n", ((byte*)buf)[i]);
			}
			else if ((i + 1) % 8 == 0) {
				target_str += sprintf(target_str, "%02hhX  ", ((byte*)buf)[i]);
			}
			else {
				target_str += sprintf(target_str, "%02hhX ", ((byte*)buf)[i]);
			}
		}
		target_str += sprintf(target_str, "\n");
		printf(full_str);
		free(full_str);
	}
	return ERROR_SUCCESS;
}

void print_square_matrix(byte matrix[MATRIX_DIMENSION][MATRIX_DIMENSION], int size_row) { //TODO probar con 3 niveles de indireccion, es mucho
//void print_square_matrix(byte** matrix, int size_row) { //TODO probar con 3 niveles de indireccion, es mucho
	/*Function for printing SQUARE matrix*/
	if (ENABLE_PRINTS) {
		int i, j;
		for (i = 0; i < size_row; i++){
			for (j = 0; j < size_row; j++){
				printf("%02X\t", matrix[i][j]);
				//if (j == size_row - 1) printf("\n");
			}
			printf("\n");
			//printf("\n_______________________________\n");
		}

	}

}

char* inputString(FILE* fp, size_t size) {
	//The size is extended by the input with the value of the provisional
	char* str;
	int ch;
	size_t len = 0;
	str = realloc(NULL, sizeof(*str) * size);//size is start size
	if (!str)return str;
	while (EOF != (ch = fgetc(fp)) && ch != '\n') {
		str[len++] = ch;
		if (len == size) {
			str = realloc(str, sizeof(*str) * (size += 16));
			if (!str)return str;
		}
	}
	str[len++] = '\0';

	return realloc(str, sizeof(*str) * len);
}

char* inputFile(FILE* fp, size_t size) {
	//The size is extended by the input with the value of the provisional
	char* str;
	int ch;
	size_t len = 0;
	str = realloc(NULL, sizeof(*str) * size);//size is start size
	if (!str)return str;
	while (EOF != (ch = fgetc(fp))){//&& ch != '\n') {
		str[len++] = ch;
		if (len == size) {
			str = realloc(str, sizeof(*str) * (size += 16));
			if (!str)return str;
		}
	}
	str[len++] = '\0';

	return realloc(str, sizeof(*str) * len);
}

/* This code is public domain -- Will Hartung 4/9/09 */
size_t getline(char** lineptr, size_t* n, FILE* stream) {
	char* bufptr = NULL;
	char* p = bufptr;
	size_t size;
	int c;

	if (lineptr == NULL) {
		return -1;
	}
	if (stream == NULL) {
		return -1;
	}
	if (n == NULL) {
		return -1;
	}
	bufptr = *lineptr;
	size = *n;

	c = fgetc(stream);
	if (c == EOF) {
		return -1;
	}
	if (bufptr == NULL) {
		bufptr = malloc(128);
		if (bufptr == NULL) {
			return -1;
		}
		size = 128;
	}
	p = bufptr;
	while (c != EOF) {
		if ((p - bufptr) > (size - 1)) {
			size = size + 128;
			bufptr = realloc(bufptr, size);
			if (bufptr == NULL) {
				return -1;
			}
		}
		*p++ = c;
		if (c == '\n') {
			break;
		}
		c = fgetc(stream);
	}

	*p++ = '\0';
	*lineptr = bufptr;
	*n = size;

	return p - bufptr - 1;
}
#pragma endregion

#pragma region Caesar dummy region
int pass_to_caesar(void* buf, int size) {
	int total = 0;
	for (int i = 0; i < size; i++) {
		total += ((byte*)buf)[i];
	}
	total = total % 255;
	printf("Numero Cesar: %d\n", total);
	return total;
}



int dummy_caesar_cypher(int caesar_index, void* plaintext, void* cipheredtext , size_t size){
	//printf("Tamaño de clave: %d\n", sizeof(message));
	//byte* cipheredtext = NULL;
	//cipheredtext = malloc(sizeof(byte) * size);
	printf("CIFRADO: Indice cesar: %d, Mensaje: %s, Tam: %d\n", caesar_index, (char*)plaintext, (int)size);
	for (int i = 0; i < size; i++) {
		((byte*)cipheredtext)[i] = (((byte*)plaintext)[i] + caesar_index) % 256;
		printf("	Por separado,( %d + %d ) mod 256 = %d.\n",((byte*)plaintext)[i], caesar_index, ((byte*)cipheredtext)[i]);
		
	}	
	PRINT_HEX(cipheredtext, size);
	return 0;
}

int dummy_caesar_decypher(int caesar_index, void* cipheredtext, void* plaintext, size_t size) {
	printf("DESCIFRADO: Indice cesar: %d, Mensaje: %s, Tam: %d\n", caesar_index, (char*)cipheredtext, (int)size);
	for (int i = 0; i < size; i++) {
		((char*)plaintext)[i] = ((((byte*)cipheredtext)[i] - caesar_index) % 256);// -1;
		printf("	Por separado, ( %d + %d ) mod 256 = %d\n", ((byte*)cipheredtext)[i], caesar_index, ((byte*)plaintext)[i]);
	}
	PRINT_HEX(plaintext, size);
	printf("Descifrado tam del texto es:%d\n", strlen(plaintext));
	return 0;
}

void dummy_caesar() {
	int caesar_index;
	char* orig_key;
	char* message;
	void* cipheredtext = NULL;
	char* plaintext = NULL;
	printf("La clave de cifrado es:\n");
    orig_key = inputString(stdin, 1);
    //printf("La clave mide %d\n", strlen(orig_key));
	//PRINT_HEX(orig_key,strlen(orig_key));
	caesar_index = pass_to_caesar(orig_key, strlen(orig_key));
	printf("Introduce el mensaje a cifrar:\n");
	message = inputString(stdin, 1);
	PRINT_HEX(message, strlen(message));
	cipheredtext = malloc(sizeof(byte) * strlen(message));
	plaintext = malloc(sizeof(char) * strlen(message));
	dummy_caesar_cypher(caesar_index, message, cipheredtext, strlen(message));
	printf("El texto cifrado es: %s\n", (char*)cipheredtext);
	dummy_caesar_decypher(caesar_index, cipheredtext, plaintext, strlen(message));
	printf("El texto descifrado es: %s, de tam: %d\n", plaintext,strlen(plaintext));
	free(cipheredtext);
	free(plaintext);

}

#pragma endregion

#pragma region Mutant Caesar
void mutant_cipher(void* message, void* ciphered, byte* keystream){
	int size = strlen((char*)message);
	for (int i = 0; i < size; i++) {
		((byte*)ciphered)[i] = (((byte*)message)[i] + keystream[i])%256;
	}
} 
void mutant_decipher(void* ciphered, void* deciphered, byte* keystream){
	int size = strlen((char*)ciphered);
	for (int i = 0; i < size; i++) {
		((byte*)deciphered)[i] = (((byte*)ciphered)[i] - keystream[i])%256;
	}
} 
void matrix_mutation(int index, byte instruction , byte matrix[MATRIX_DIMENSION][MATRIX_DIMENSION] ){ //TODO Muchas mas transformaciones
	if (instruction % 2 == 0) {
		matrix[0][0] = (matrix[0][0] + 1) % 256;
		matrix[1][0] = (matrix[1][0] + 1) % 256;
		matrix[2][0] = (matrix[2][0] + 1) % 256;
		matrix[3][0] = (matrix[3][0] + 1) % 256;

		matrix[0][2] = (matrix[0][2] + 1) % 256;
		matrix[1][2] = (matrix[1][2] + 1) % 256;
		matrix[2][2] = (matrix[2][2] + 1) % 256;
		matrix[3][2] = (matrix[3][2] + 1) % 256;
	}
	else {
		matrix[0][1] = (matrix[0][1] + 2) % 256;
		matrix[1][1] = (matrix[1][1] + 2) % 256;
		matrix[2][1] = (matrix[2][1] + 2) % 256;
		matrix[3][1] = (matrix[3][1] + 2) % 256;

		matrix[0][3] = (matrix[0][3] + 2) % 256;
		matrix[1][3] = (matrix[1][3] + 2) % 256;
		matrix[2][3] = (matrix[2][3] + 2) % 256;
		matrix[3][3] = (matrix[3][3] + 2) % 256;
	}
}

void mutant_caesar() {
	char* key;
	char* message;
	void* cipheredtext = NULL;
	char* plaintext = NULL;
	byte keymatrix[MATRIX_DIMENSION][MATRIX_DIMENSION] = { { 0x00 } };
	int index_key = 0;
	byte instructions[4];
	
	printf("Enter encryption key:\n");
	key = inputString(stdin, 1);
	
	//INIT KEYMATRIX

	/*byte** keymatrix = malloc(4 * sizeof(*keymatrix));
	for (int k = 0; k < 4; k++) {
		keymatrix[k] = malloc(4 * sizeof(*keymatrix[k]));
	}*/

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			keymatrix[i][j] = key[index_key];
			index_key = (index_key + 1) % strlen(key);
		}
	}
	//PRINT_HEX(keymatrix, KEYSIZE);
	printf("\n");
	PRINT_SMATRIX(keymatrix, 4);

	//GET INSTRUCTIONS: LAST COLUMN
	instructions[0] = keymatrix[0][3];
	instructions[1] = keymatrix[1][3];
	instructions[2] = keymatrix[2][3];
	instructions[3] = keymatrix[3][3];
	PRINT("\nInstructions: ");
	PRINT_HEX(instructions, 4);

	//GET MESSAGE TO CYPHER
	PRINT("Enter message:\n");
	message = inputString(stdin, 1);
	int tam = strlen(message);

	//byte keystream[10] = { 0 };
	byte* keystream = malloc(tam * sizeof(byte));
	//PRODUCE KEYSTREAM
	for (int i = 0; i < tam; i++) {
		matrix_mutation(i, instructions[i % 4], keymatrix);
		PRINT("Iteracion %d la instruccion es %02X\n",i,instructions[i%4]);
		PRINT_SMATRIX(keymatrix, 4);
		keystream[i] = keymatrix[(i / 4) % 4][i % 4];
		PRINT("Se coge el valor de la matrix %d,%d = %02X\n\n", (i / 4) % 4, i % 4, keymatrix[(i / 4) % 4][i % 4]);
	}
	PRINT("Keystream del mensaje: ");
	PRINT_HEX(keystream, tam);
	
	PRINT("\n");
	PRINT("MENSAJE ORIGINAL: ");
	PRINT_HEX(message, tam);
	PRINT("\nA continuacion cifro\n");
	byte* ciphered = malloc(tam * sizeof(byte));
	mutant_cipher(message, ciphered, keystream);
	PRINT("Mensaje cifrado: ");
	PRINT_HEX(ciphered, tam);
	PRINT("\nPor ultimo descifro\n");
	byte* deciphered = malloc(tam * sizeof(byte));
	mutant_decipher(ciphered, deciphered, keystream);
	PRINT("Mensaje descifrado: ");
	PRINT_HEX(deciphered, tam);

	//TODO frees y cifrar o descifrar en ciertas posiciones
	free(message);
	free(key);
	free(ciphered);
	free(deciphered);

}
#pragma endregion

#pragma region Mutant caesar with improvements
int get_next_index(int old_index){
	//Si el tamaño de la key es par, le mando fibonacci
	//Si el tamaño de la key es impar le mando el siguiente
}

void mutant_cipher_develop(void* message, void* ciphered, byte* keystream) {
	int size = strlen((char*)message);
	for (int i = 0; i < size; i++) {
		((byte*)ciphered)[i] = (((byte*)message)[i] + keystream[i]) % 256;
	}
}
void mutant_decipher_develop(void* ciphered, void* deciphered, byte* keystream) {
	int size = strlen((char*)ciphered);
	for (int i = 0; i < size; i++) {
		((byte*)deciphered)[i] = (((byte*)ciphered)[i] - keystream[i]) % 256;
	}
}
void matrix_mutation_develop(int index, byte instruction, byte matrix[MATRIX_DIMENSION][MATRIX_DIMENSION]) { //TODO Muchas mas transformaciones
	if (instruction % 2 == 0) {
		//sum_1_pair_columns(byte matrix[MATRIX_DIMENSION][MATRIX_DIMENSION])
		matrix[0][0] = (matrix[0][0] + 1) % 256;
		matrix[1][0] = (matrix[1][0] + 1) % 256;
		matrix[2][0] = (matrix[2][0] + 1) % 256;
		matrix[3][0] = (matrix[3][0] + 1) % 256;

		matrix[0][2] = (matrix[0][2] + 1) % 256;
		matrix[1][2] = (matrix[1][2] + 1) % 256;
		matrix[2][2] = (matrix[2][2] + 1) % 256;
		matrix[3][2] = (matrix[3][2] + 1) % 256;
	}
	else {
		//sum_2_impair_columns(byte matrix[MATRIX_DIMENSION][MATRIX_DIMENSION])
		matrix[0][1] = (matrix[0][1] + 2) % 256;
		matrix[1][1] = (matrix[1][1] + 2) % 256;
		matrix[2][1] = (matrix[2][1] + 2) % 256;
		matrix[3][1] = (matrix[3][1] + 2) % 256;

		matrix[0][3] = (matrix[0][3] + 2) % 256;
		matrix[1][3] = (matrix[1][3] + 2) % 256;
		matrix[2][3] = (matrix[2][3] + 2) % 256;
		matrix[3][3] = (matrix[3][3] + 2) % 256;
	}
}

void mutant_caesar_develop() {
	char* key;
	char* message;
	void* cipheredtext = NULL;
	char* plaintext = NULL;
	byte keymatrix[MATRIX_DIMENSION][MATRIX_DIMENSION] = { { 0x00 } };
	int index_key = 0;
	byte instructions[4];

	printf("Enter encryption key:\n");
	key = inputString(stdin, 1);

	//INIT KEYMATRIX

	/*byte** keymatrix = malloc(4 * sizeof(*keymatrix));
	for (int k = 0; k < 4; k++) {
		keymatrix[k] = malloc(4 * sizeof(*keymatrix[k]));
	}*/

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			keymatrix[i][j] = key[index_key];
			index_key = (index_key + 1) % strlen(key);
		}
	}
	//PRINT_HEX(keymatrix, KEYSIZE);
	printf("\n");
	PRINT_SMATRIX(keymatrix, 4);

	//GET INSTRUCTIONS: LAST COLUMN
	instructions[0] = keymatrix[0][3];
	instructions[1] = keymatrix[1][3];
	instructions[2] = keymatrix[2][3];
	instructions[3] = keymatrix[3][3];
	PRINT("\nInstructions: ");
	PRINT_HEX(instructions, 4);

	//GET MESSAGE TO CYPHER
	PRINT("Enter message:\n");
	message = inputString(stdin, 1);
	int tam = strlen(message);

	//byte keystream[10] = { 0 };
	byte* keystream = malloc(tam * sizeof(byte));
	//DESORDENADO INICIAL
	int disorder_factor = keymatrix[0][0] + keymatrix[1][0] + keymatrix[2][0] + keymatrix[3][0];
	PRINT("El factor de desorden es: %d, es decir la matriz empezara tras %d desordenaciones consecutivas\n", disorder_factor, disorder_factor);
	for (int i = 0; i < disorder_factor; i++) {
		matrix_mutation(i, instructions[i % 4], keymatrix);
		//Mutacion de instrucciones cada 4 variaciones en la matriz
		if (i % 4 == 0) {
			instructions[0] = keymatrix[0][3];
			instructions[1] = keymatrix[1][3];
			instructions[2] = keymatrix[2][3];
			instructions[3] = keymatrix[3][3];
		}
	}
	//PRODUCE KEYSTREAM
	for (int i = 0; i < tam; i++) {
		matrix_mutation(i, instructions[i % 4], keymatrix);
		PRINT("Iteracion %d la instruccion es %02X\n", i, instructions[i % 4]);
		PRINT_SMATRIX(keymatrix, 4);
		keystream[i] = keymatrix[(i / 4)%4][i % 4];
		PRINT("Se coge el valor de la matrix %d,%d = %02X\n\n", (i / 4)%4, i % 4, keymatrix[(i / 4)%4][i % 4]);
		//Mutacion de instrucciones cada 4 variaciones en la matriz
		if (i % 4 == 0) {
			instructions[0] = keymatrix[0][3];
			instructions[1] = keymatrix[1][3];
			instructions[2] = keymatrix[2][3];
			instructions[3] = keymatrix[3][3];
			PRINT("\nSe vuelven a coger las instrucciones: ");
			PRINT_HEX(instructions, 4);
		}
	}
	PRINT("Keystream del mensaje: ");
	PRINT_HEX(keystream, tam);

	PRINT("\n");
	PRINT("MENSAJE ORIGINAL: ");
	PRINT_HEX(message, tam);
	PRINT("\nA continuacion cifro\n");
	byte* ciphered = malloc(tam * sizeof(byte));
	mutant_cipher(message, ciphered, keystream);
	PRINT("Mensaje cifrado: ");
	PRINT_HEX(ciphered, tam);
	PRINT("\nPor ultimo descifro\n");
	byte* deciphered = malloc(tam * sizeof(byte));
	mutant_decipher(ciphered, deciphered, keystream);
	PRINT("Mensaje descifrado: ");
	PRINT_HEX(deciphered, tam);

	//TODO frees y cifrar o descifrar en ciertas posiciones
	free(message);
	free(key);
	free(ciphered);
	free(deciphered);

}
#pragma endregion

#pragma region Mutant Caesar With Searching
void mutant_cipher_search(void* message, void* ciphered, byte* keystream) {
	int size = strlen((char*)message);
	for (int i = 0; i < size; i++) {
		((byte*)ciphered)[i] = (((byte*)message)[i] + keystream[i]) % 256;
	}
}
void mutant_decipher_search(void* ciphered, void* deciphered, byte* keystream) {
	int size = strlen((char*)ciphered);
	for (int i = 0; i < size; i++) {
		((byte*)deciphered)[i] = (((byte*)ciphered)[i] - keystream[i]) % 256;
	}
}

void simple_op_matrix(byte matrix[MATRIX_DIMENSION][MATRIX_DIMENSION], enum SIMPLE_OPERATION op, int value, int col1, int col2) {
	PRINT("Se %s %d a las columnas %d y %d\n", (op==ADD)?"suma":"resta", value, col1, col2);
	for (int i = 0; i < MATRIX_DIMENSION; i++) {
		//(byte)matrix[i][col1] = (op == ADD) ? (byte)(matrix[i][col1] + value % 256) : (byte)(matrix[i][col1] - value % 256);
		matrix[i][col1] = (op == ADD) ? matrix[i][col1] + value % 256 : matrix[i][col1] - value % 256;
		//(byte)matrix[i][col2] = (op == ADD) ? (byte)(matrix[i][col2] + value % 256) : (byte)(matrix[i][col2] - value % 256);
		matrix[i][col2] = (op == ADD) ? matrix[i][col2] + value % 256 : matrix[i][col2] - value % 256;
		//PRINT2("Se ha operado en %d,%d = %02X y en %d,%d = %02X\n", i,col1, (byte)matrix[i][col1], i, col2, (byte)matrix[i][col2]);
	}
}
void matrix_mutation_search(int index, byte instruction, byte matrix[MATRIX_DIMENSION][MATRIX_DIMENSION]) { //TODO Muchas mas transformaciones
	if (instruction % 7 == 0) simple_op_matrix(matrix, SUB, 3, 0, 2);//sub3_columns02(matrix);
	else if (instruction % 5 == 0) simple_op_matrix(matrix, ADD, 4, 2, 3);//sum4_columns34(matrix);
	else if (instruction % 3 == 0) simple_op_matrix(matrix, ADD, 1, 0, 1);//sum1_columns01(matrix);
	else if (instruction % 2 == 0) simple_op_matrix(matrix, ADD, 1, 0, 2);
	else simple_op_matrix(matrix, ADD, 2, 1, 3);
}

void mutant_caesar_search() {
	char* key;
	char* message;
	void* cipheredtext = NULL;
	char* plaintext = NULL;
	byte keymatrix[MATRIX_DIMENSION][MATRIX_DIMENSION] = { { 0x00 } };
	int index_key = 0;
	byte instructions[4];

	printf("Enter encryption key:\n");
	key = inputString(stdin, 1);

	//INIT KEYMATRIX

	/*byte** keymatrix = malloc(4 * sizeof(*keymatrix));
	for (int k = 0; k < 4; k++) {
		keymatrix[k] = malloc(4 * sizeof(*keymatrix[k]));
	}*/

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			keymatrix[i][j] = key[index_key];
			index_key = (index_key + 1) % strlen(key);
		}
	}
	//PRINT_HEX(keymatrix, KEYSIZE);
	printf("\n");
	PRINT_SMATRIX(keymatrix, 4);

	//GET INSTRUCTIONS: LAST COLUMN
	instructions[0] = keymatrix[0][3];
	instructions[1] = keymatrix[1][3];
	instructions[2] = keymatrix[2][3];
	instructions[3] = keymatrix[3][3];
	PRINT("\nInstructions: ");
	PRINT_HEX(instructions, 4);

	//CALCULAR MATRIZ LEJANA
	byte new_keymatrix[4][4] = { 0 }; //TODO HACER ESTO BIEN
	new_keymatrix[0][0] = keymatrix[0][0];
	new_keymatrix[1][0] = keymatrix[1][0];
	new_keymatrix[2][0] = keymatrix[2][0];
	new_keymatrix[3][0] = keymatrix[3][0];
	new_keymatrix[0][1] = keymatrix[0][1];
	new_keymatrix[1][1] = keymatrix[1][1];
	new_keymatrix[2][1] = keymatrix[2][1];
	new_keymatrix[3][1] = keymatrix[3][1];
	new_keymatrix[0][2] = keymatrix[0][2];
	new_keymatrix[1][2] = keymatrix[1][2];
	new_keymatrix[2][2] = keymatrix[2][2];
	new_keymatrix[3][2] = keymatrix[3][2];
	new_keymatrix[0][3] = keymatrix[0][3];
	new_keymatrix[1][3] = keymatrix[1][3];
	new_keymatrix[2][3] = keymatrix[2][3];
	new_keymatrix[3][3] = keymatrix[3][3];
	PRINT("Que matriz quieres calcular?\n");
	int matrix_num = atoi(inputString(stdin, 1));
	PRINT("Vamos a calcularla de manera tradicional");
	//PRODUCE KEYSTREAM
	for (int i = 0; i < matrix_num; i++) {
		matrix_mutation_search(i, instructions[i % 4], keymatrix);
		PRINT("Iteracion %d la instruccion es %02X\n", i, instructions[i % 4]);
		PRINT_SMATRIX(keymatrix, 4);
		//keystream[i] = keymatrix[(i / 4) % 4][i % 4];
		//PRINT("Se coge el valor de la matrix %d,%d = %02X\n\n", (i / 4) % 4, i % 4, keymatrix[(i / 4) % 4][i % 4]);
	}
	PRINT("LA MATRIZ TRAS LAS ITERACIONES ES: \n");
	PRINT_SMATRIX(keymatrix, 4);
	//PRODUCE ACC KEYSTREAM: Primero calculo cuantas tengo que hacer de cada categoria, luego hago todas de golpe.
	int repetitions[5] = { 0 };
	PRINT("Antes del calculo de repeticiones quedan %d,%d,%d,%d,%d\n", repetitions[0], repetitions[1], repetitions[2], repetitions[3], repetitions[4]);
	for (int i = 0; i < matrix_num; i++) {
		if (instructions[i%4] % 7 == 0) repetitions[i]++;
		else if (instructions[i%4] % 5 == 0) repetitions[i]++;
		else if (instructions[i%4] % 3 == 0) repetitions[i]++;
		else if (instructions[i%4] % 2 == 0) repetitions[i]++;
		else repetitions[i%4]++;
	}
	PRINT("Tras el calculo de repeticiones quedan %d,%d,%d,%d,%d\n", repetitions[0], repetitions[1], repetitions[2], repetitions[3], repetitions[4]);
	for (int i = 0; i < 5; i++) {
		switch (i) {
			case 0:
				simple_op_matrix(new_keymatrix, SUB, 3*repetitions[i], 0, 2); //Es del tipo divisible por 7
				break;
			case 1:
				simple_op_matrix(new_keymatrix, ADD, 4 * repetitions[i], 2, 3); //Es del tipo divisible por 5
				break;
			case 2:
				simple_op_matrix(new_keymatrix, ADD, 1 * repetitions[i], 0, 1); //Es del tipo divisible por 3
				break;
			case 3:
				simple_op_matrix(new_keymatrix, ADD, 1 * repetitions[i], 0, 2); //Es del tipo divisible por 2
				break;
			default:
				simple_op_matrix(new_keymatrix, ADD, 2 * repetitions[i], 1, 3); //Caso default
				break;
		}
	}

	PRINT("LA MATRIZ CON SEARCH ES: \n");
	PRINT_SMATRIX(new_keymatrix, 4);
	//PRINT("Keystream del mensaje: ");
	//PRINT_HEX(keystream, tam);

	//GET MESSAGE TO CYPHER
	/*PRINT("Enter message:\n");
	message = inputString(stdin, 1);
	int tam = strlen(message);

	//byte keystream[10] = { 0 };
	byte* keystream = malloc(tam * sizeof(byte));
	//PRODUCE KEYSTREAM
	for (int i = 0; i < tam; i++) {
		matrix_mutation_search(i, instructions[i % 4], keymatrix);
		PRINT("Iteracion %d la instruccion es %02X\n", i, instructions[i % 4]);
		PRINT_SMATRIX(keymatrix, 4);
		keystream[i] = keymatrix[(i / 4) % 4][i % 4];
		PRINT("Se coge el valor de la matrix %d,%d = %02X\n\n", (i / 4) % 4, i % 4, keymatrix[(i / 4) % 4][i % 4]);
	}
	PRINT("Keystream del mensaje: ");
	PRINT_HEX(keystream, tam);

	PRINT("\n");
	PRINT("MENSAJE ORIGINAL: ");
	PRINT_HEX(message, tam);
	PRINT("\nA continuacion cifro\n");
	byte* ciphered = malloc(tam * sizeof(byte));
	mutant_cipher_search(message, ciphered, keystream);
	PRINT("Mensaje cifrado: ");
	PRINT_HEX(ciphered, tam);
	PRINT("\nPor ultimo descifro\n");
	byte* deciphered = malloc(tam * sizeof(byte));
	mutant_decipher_search(ciphered, deciphered, keystream);
	PRINT("Mensaje descifrado: ");
	PRINT_HEX(deciphered, tam);

	//TODO frees y cifrar o descifrar en ciertas posiciones
	//PRINT("CLEAN BUFFERS\n");
	//free(message);
	//free(key);
	//free(keystream);
	free(ciphered);
	free(deciphered);*/

}
#pragma endregion

#pragma region Mutant caesar with 64 bytes matrix
int get_next_index_64(int old_index) {
	//Si el tamaño de la key es par, le mando fibonacci
	//Si el tamaño de la key es impar le mando el siguiente
}

void mutant_cipher_64(void* message, void* ciphered, byte* keystream, int size) {
	//int size = strlen((char*)keystream);

	//PRINT2("Tam de cifrado: %d\n", size);
	for (int i = 0; i < size; i++) {
		((byte*)ciphered)[i] = (((byte*)message)[i] + keystream[i]) % 256;
		//((char*)ciphered)[i] = (((char*)message)[i] ^ keystream[i]) % 256;
	}
	//PRINT2("Tam despues de cifrado: %d\n", strlen((char*)ciphered));
}
void mutant_decipher_64(void* ciphered, void* deciphered, byte* keystream, int size) {
	//int size = strlen((char*)keystream);
	//PRINT2("Tam de descifrado: %d\n", size);
	for (int i = 0; i < size; i++) {
		((byte*)deciphered)[i] = (((byte*)ciphered)[i] - keystream[i]) % 256;
		//((char*)deciphered)[i] = (((char*)ciphered)[i] ^ keystream[i]) % 256;
	}
	//PRINT2("Tam despues de descifrado: %d\n", strlen((char*)deciphered));
}
void matrix_mutation_64(int index, byte instruction, byte matrix[MATRIX_DIMENSION_64][MATRIX_DIMENSION_64]) { //TODO Muchas mas transformaciones
	int row = 0;
	int col = 0;
	byte tmp;
	BOOL enc;
	int ninst = 7; //Tipos de instrucciones
	if ((int)instruction% ninst ==0){
	//if (0x00 >= instruction && instruction <= 0x24) { //sum n to pair columns
	//if (0x00 >= instruction && instruction <= 0x64) { //sum n to pair columns
		for (int i = 0; i < MATRIX_DIMENSION_64 / 2; i += 2) {
			for (int j = 0; j < MATRIX_DIMENSION_64; j++) {
				matrix[j][i] = (matrix[j][i] + 1) % 256;
			}
		}
	}
	if ((int)instruction % ninst == 1) {
	//if (0x25 > instruction && instruction <= 0x48){
	//if (0x64 > instruction && instruction <= 0x80) { //sum n to impair columns
		for (int i = 1; i < MATRIX_DIMENSION_64 / 2; i += 2) {
			for (int j = 0; j < MATRIX_DIMENSION_64; j++) {
				matrix[j][i] = (matrix[j][i] + 1) % 256;
			}
		}
	}
	if ((int)instruction % ninst == 2) {
	//if (0x48 > instruction && instruction <= 0x6C) {
	//if (0x80 > instruction && instruction <= 0xC0) { //sum n to pair rows
		for (int i = 1; i < MATRIX_DIMENSION_64 / 2; i += 2) {
			for (int j = 0; j < MATRIX_DIMENSION_64; j++) {
				matrix[i][j] = (matrix[i][j] + 1) % 256;
			}
		}
	}
	if ((int)instruction % ninst == 3) {
	//if (0x6C > instruction && instruction <= 0x90) {
	//if (0xC0 > instruction && instruction <= 0xFF) { //sum n to impair rows
		for (int i = 1; i < MATRIX_DIMENSION_64 / 2; i += 2) {
			for (int j = 0; j < MATRIX_DIMENSION_64; j++) {
				matrix[i][j] = (matrix[i][j] + 1) % 256;
			}
		}
	}
	if ((int)instruction % ninst == 4) {
	//if (0x90 > instruction && instruction <= 0xB4) {
	//if (0 == 1) { //shift columns to left
		for (int i = 0; i < MATRIX_DIMENSION_64 - 1; i++) {
			//guardo en temp la columna i+1 mod MATRIX_DIMENSION y hago meto i en i+1
			for (int j = 0; j < MATRIX_DIMENSION_64 - 1; j++) {
				tmp = matrix[j][i];
				matrix[j][i] = matrix[j][i + 1];
				matrix[j][i + 1] = tmp;
			}
		}
	}
	if ((int)instruction % ninst == 5) {
	//if (0xB4 > instruction && instruction <= 0xD8) {
	//if (0 == 1) { //shift rows down
		for (int i = 0; i < MATRIX_DIMENSION_64-1; i++) {
			for (int j = 0; j < MATRIX_DIMENSION_64-1; j++) {
				tmp = matrix[i][j];
				matrix[i][j] = matrix[i][j + 1];
				matrix[i][j + 1] = tmp;
			}
		}
	}
	if ((int)instruction % ninst == 6) {
	//if (0xD8 > instruction && instruction <= 0xFF) {
	//if (0==1) { //switch columns/rows
		for (int i = 0; i < MATRIX_DIMENSION_64-1; i ++) {
			for (int j = i; j < MATRIX_DIMENSION_64-1; j++) {
				tmp = matrix[i][j];
				matrix[i][j] = matrix[j][i];
				matrix[i][j] = tmp;
			}
		}
	}
}

void mutant_caesar_64() {
	char* key;
	char* message;
	void* cipheredtext = NULL;
	char* plaintext = NULL;
	byte keymatrix[MATRIX_DIMENSION_64][MATRIX_DIMENSION_64] = { { 0x00 } };
	int index_key = 0;
	byte instructions[MATRIX_DIMENSION_64];

	printf("Enter encryption key:\n");
	key = inputString(stdin, 1);

	//INIT KEYMATRIX

	for (int i = 0; i < MATRIX_DIMENSION_64; i++) {
		for (int j = 0; j < MATRIX_DIMENSION_64; j++) {
			keymatrix[i][j] = key[index_key];
			index_key = (index_key + 1) % strlen(key);
		}
	}
	//PRINT_HEX(keymatrix, KEYSIZE);
	printf("\n");
	PRINT_SMATRIX(keymatrix, MATRIX_DIMENSION_64);

	//GET INSTRUCTIONS: LAST COLUMN
	for (int i = 0; i < MATRIX_DIMENSION_64; i++) {
		instructions[i] = keymatrix[i][MATRIX_DIMENSION_64 - 1];
	}
	PRINT("\nInstructions: ");
	PRINT_HEX(instructions, MATRIX_DIMENSION_64);

	//GET MESSAGE TO CYPHER
	PRINT("Enter message:\n");
	//FILE* fp = NULL;
	//char buff[20]; //Realocar bien el buffer
	//fp = fopen("el_quijote.txt", "r");
	//fgets(buff, 20, fp);
	message = inputString(stdin, 1);
	//message = buff;
	//message = inputFile(fp, 0);
	int tam = strlen(message);
	PRINT("%s\n", message);

	byte* keystream = malloc(tam * sizeof(byte));

	//DESORDENADO INICIAL
	/*int disorder_factor = keymatrix[0][0] + keymatrix[1][0] + keymatrix[2][0] + keymatrix[3][0];
	PRINT("El factor de desorden es: %d, es decir la matriz empezara tras %d desordenaciones consecutivas\n", disorder_factor, disorder_factor);
	for (int i = 0; i < disorder_factor; i++) {
		matrix_mutation_64(i, instructions[i % MATRIX_DIMENSION_64], keymatrix);
		//Mutacion de instrucciones cada 4 variaciones en la matriz
		if (i % MATRIX_DIMENSION_64 == 0) {
			instructions[0] = keymatrix[0][7];
			instructions[1] = keymatrix[1][7];
			instructions[2] = keymatrix[2][7];
			instructions[3] = keymatrix[3][7];
			instructions[4] = keymatrix[4][7];
			instructions[5] = keymatrix[5][7];
			instructions[6] = keymatrix[6][7];
			instructions[7] = keymatrix[7][7];
		}
	}*/

	//PRODUCE KEYSTREAM
	for (int i = 0; i < tam; i++) {
		matrix_mutation_64(i, instructions[i % MATRIX_DIMENSION_64], keymatrix);
		PRINT("Iteracion %d la instruccion es %02X\n", i, instructions[i % 4]);
		PRINT_SMATRIX(keymatrix, MATRIX_DIMENSION_64);
		keystream[i] = keymatrix[(i / MATRIX_DIMENSION_64) % MATRIX_DIMENSION_64][i % MATRIX_DIMENSION_64];
		PRINT("Se coge el valor de la matrix %d,%d = %02X\n\n", (i / MATRIX_DIMENSION_64) % MATRIX_DIMENSION_64, i % MATRIX_DIMENSION_64, keymatrix[(i / MATRIX_DIMENSION_64) % MATRIX_DIMENSION_64][i % MATRIX_DIMENSION_64]);
		//Mutacion de instrucciones cada 4 variaciones en la matriz
		if (i % MATRIX_DIMENSION_64 == 0) {
			instructions[0] = keymatrix[0][7];
			instructions[1] = keymatrix[1][7];
			instructions[2] = keymatrix[2][7];
			instructions[3] = keymatrix[3][7];
			instructions[4] = keymatrix[4][7];
			instructions[5] = keymatrix[5][7];
			instructions[6] = keymatrix[6][7];
			instructions[7] = keymatrix[7][7];
			PRINT("\nSe vuelven a coger las instrucciones: ");
			PRINT_HEX(instructions, MATRIX_DIMENSION_64);
		}
	}
	PRINT("Keystream del mensaje: ");
	PRINT_HEX(keystream, tam);

	PRINT("\n");
	PRINT("MENSAJE ORIGINAL: ");
	PRINT_HEX(message, tam);
	PRINT("\nA continuacion cifro\n");
	byte* ciphered = malloc(tam * sizeof(byte));
	mutant_cipher(message, ciphered, keystream);
	PRINT("Mensaje cifrado: ");
	PRINT_HEX(ciphered, tam);
	PRINT("\nPor ultimo descifro\n");
	byte* deciphered = malloc(tam * sizeof(byte));
	mutant_decipher(ciphered, deciphered, keystream);
	PRINT("Mensaje descifrado: ");
	PRINT_HEX(deciphered, tam);

	//TODO frees y cifrar o descifrar en ciertas posiciones
	free(message);
	free(key);
	free(ciphered);
	free(deciphered);
}


void mutant_caesar_file() {
	char* key;
	char* message;
	void* cipheredtext = NULL;
	char* plaintext = NULL;
	byte keymatrix[MATRIX_DIMENSION_64][MATRIX_DIMENSION_64] = { { 0x00 } };
	int index_key = 0;
	byte instructions[MATRIX_DIMENSION_64];
	
	printf("Enter encryption key:\n");
	key = inputString(stdin, 1);

	//INIT KEYMATRIX

	for (int i = 0; i < MATRIX_DIMENSION_64; i++) {
		for (int j = 0; j < MATRIX_DIMENSION_64; j++) {
			keymatrix[i][j] = key[index_key];
			index_key = (index_key + 1) % strlen(key);
		}
	}
	//PRINT_HEX(keymatrix, KEYSIZE);
	printf("\n");
	//PRINT_SMATRIX(keymatrix, MATRIX_DIMENSION_64);
	
	//GET INSTRUCTIONS: LAST COLUMN OR DIAGONAL PRINCIPAL
	for (int i = 0; i < MATRIX_DIMENSION_64; i++) {
		//instructions[i]= keymatrix[i][MATRIX_DIMENSION_64-1];
		instructions[i] = keymatrix[i][i];
	}
	PRINT("\nInstructions: ");
	PRINT_HEX(instructions, MATRIX_DIMENSION_64);
	
	//GET MESSAGE TO CYPHER
	PRINT("Enter message:\n");
	FILE* fp = NULL;
	//char buff[20]; //Realocar bien el buffer
	fp = fopen("el_quijote.txt", "r");
	FILE* fpout = NULL;
	fpout = fopen("fichero_cifrado.txt", "a");
	FILE* fpout2 = NULL;
	fpout2 = fopen("fichero_descifrado.txt", "a");
	message = inputFile(fp,1);
	int tam = strlen(message);
	//PRINT("Mensaje de tam: %d	%s\n", tam, message);
	PRINT("Mensaje de tam: %d\n", tam);
	clock_t start = clock();
	byte* keystream = malloc(tam * sizeof(byte));

	//DESORDENADO INICIAL
	int disorder_factor = 0;// keymatrix[0][0] + keymatrix[1][0] + keymatrix[2][0] + keymatrix[3][0];
	for (int i = 0; i < MATRIX_DIMENSION_64; i++) {
		//instructions[i]= keymatrix[i][MATRIX_DIMENSION_64-1];
		disorder_factor += keymatrix[i][i];
	}
	PRINT("El factor de desorden es: %d, es decir la matriz empezara tras %d desordenaciones consecutivas\n", disorder_factor, disorder_factor);
	for (int i = 0; i < disorder_factor; i++) {
		matrix_mutation_64(i, instructions[i % MATRIX_DIMENSION_64], keymatrix);
		//Mutacion de instrucciones cada 4 variaciones en la matriz
		if (i % MATRIX_DIMENSION_64 == 0) {
			for (int i = 0; i < MATRIX_DIMENSION_64; i++) {
				//instructions[i]= keymatrix[i][MATRIX_DIMENSION_64-1];
				instructions[i] = keymatrix[i][i];
			}
		}
	}

	//PRODUCE KEYSTREAM
	for (int i = 0; i < tam; i++) {
		matrix_mutation_64(i, instructions[i % MATRIX_DIMENSION_64], keymatrix);
		//PRINT("Iteracion %d la instruccion es %02X\n", i, instructions[i % 4]);
		//PRINT_SMATRIX(keymatrix, MATRIX_DIMENSION_64);
		keystream[i] = keymatrix[(i / MATRIX_DIMENSION_64) % MATRIX_DIMENSION_64][i % MATRIX_DIMENSION_64];
		//PRINT("Se coge el valor de la matrix %d,%d = %02X\n\n", (i / MATRIX_DIMENSION_64) % MATRIX_DIMENSION_64, i % MATRIX_DIMENSION_64, keymatrix[(i / MATRIX_DIMENSION_64) % MATRIX_DIMENSION_64][i % MATRIX_DIMENSION_64]);
		//Mutacion de instrucciones cada 4 variaciones en la matriz
		if (i % MATRIX_DIMENSION_64 == 0) {
			for (int i = 0; i < MATRIX_DIMENSION_64; i++) {
				//instructions[i]= keymatrix[i][MATRIX_DIMENSION_64-1];
				instructions[i] = keymatrix[i][i];
			}
			//PRINT("\nSe vuelven a coger las instrucciones: ");
			//PRINT_HEX(instructions, MATRIX_DIMENSION_64);
		}
	}

	//PRINT("Keystream del mensaje: ");
	//PRINT_HEX(keystream, tam);
	/*for (int i = 0; i < tam; i++) {
		PRINT("Iteracion: %d, valor de keystream: %02hhX\n", i, keystream[i]);
	}
	PRINT_HEX(keystream, tam);*/

	//PRINT("\n");
	//PRINT("MENSAJE ORIGINAL: ");
	//PRINT_HEX(message, tam);
	clock_t end = clock();
	PRINT("\nA continuacion cifro los %d bytes\n", tam);
	char* ciphered = malloc(tam * sizeof(char));
	mutant_cipher_64(message, ciphered, keystream, tam);
	//clock_t end = clock();
	//PRINT("Mensaje cifrado: ");
	//PRINT_HEX(ciphered, tam);
	PRINT("\nPor ultimo descifro los %d bytes\n", tam);
	char* deciphered = malloc(tam * sizeof(char));
	mutant_decipher_64(ciphered, deciphered, keystream, tam);
	//PRINT("Mensaje descifrado: ");
	//PRINT_HEX(deciphered, tam);
	//fputs(ciphered, fpout);
	//fputs(deciphered, fpout2);
	PRINT("Escribo los bytes cifrados: %d\n",fwrite(ciphered, 1, tam, fpout));
	PRINT("Escribo los bytes descifrados: %d\n", fwrite(deciphered, 1, tam, fpout2));
	fclose(fpout);
	fclose(fpout2);
	fclose(fp);
	//TODO frees y cifrar o descifrar en ciertas posiciones
	free(message);
	free(key);
	free(ciphered);
	free(deciphered);
	float seconds = (float)(end - start) / CLOCKS_PER_SEC;
	PRINT("He tardado en cifrar %.16f segundos\n",seconds);

}

void swap(unsigned char* a, unsigned char* b) {
	int tmp = *a;
	*a = *b;
	*b = tmp;
}

//KSA: Key scheduling algorithm
int ksa(char* key, unsigned char* S) {
	int i = 0;
	int keylen = strlen(key);
	//Primero se rellena el array de 256 bytes
	for (i = 0; i < Sbox_size; i++) {
		S[i] = i;
	}
	//A continuacion se intercambian sus valores mezclandolos con la clave
	int j = 0;
	for (i = 0; i < Sbox_size; i++) {
		//La key funciona como semilla que marca los intercambios en S
		j = (j + S[i] + key[i % keylen]) % Sbox_size;
		swap(&S[i], &S[j]);
	}
	return 0;
}

//PRGA: Pseudo Random generation algorithm
int prga(unsigned char* S, char* orig, unsigned char* ciphered) {
	int i = 0, j = 0, n = 0, rnd = 0;

	for (n = 0; n < strlen(orig); n++) {
		i = (i + 1) % Sbox_size;
		j = (j + S[i]) % Sbox_size;

		swap(&S[i], &S[j]);
		rnd = S[(S[i] + S[j]) % Sbox_size]; //rnd es cada byte del keystream

		//El elemento cifrado, es el elemento elegido de S xor Texto plano
		ciphered[n] = rnd ^ orig[n];

	}
	return 0;
}

//RC4
int rc4(char* key, char* orig, unsigned char* ciphered) {
	int i = 0;
	unsigned char S[Sbox_size];

	ksa(key, S);
	prga(S, orig, ciphered);

	return 0;
}

int cipher_rc4() {
	char* test_vectors[3][2] =
	{
		{"Key", "Plaintext"},
		{"Wiki", "pedia"},
		{"Secret", "Attack at dawn"}
	};

	printf("Resultados del conjunto de evaluacion\n");

	for (int i = 0; i < 3; i++) {
		printf("    Texto plano: %s, Clave: %s, Texto cifrado: ", test_vectors[i][1], test_vectors[i][0]);
		unsigned char* ciphered = malloc(sizeof(int) * strlen(test_vectors[i][1]));
		rc4(test_vectors[i][0], test_vectors[i][1], ciphered);
		for (int j = 0; j < strlen(test_vectors[i][1]); j++) {
			printf("%02X", ciphered[j]);
		}
		printf("\n");
	}
	//TRANSLATE QUIXOTE
	char* message;
	char* key;
	FILE* fp = NULL;
	fp = fopen("el_quijote.txt", "r");
	FILE* fpout = NULL;
	fpout = fopen("fichero_cifradorc4.txt", "a");
	//FILE* fpout2 = NULL;
	//fpout2 = fopen("fichero_descifrado.txt", "a");
	message = inputFile(fp, 1);
	int tam = strlen(message);

	char* ciphered = malloc(tam * sizeof(char));
	printf("Enter encryption key:\n");
	key = inputString(stdin, 1);
	rc4(key, message, ciphered);
	PRINT("Escribo los bytes cifrados: %d\n", fwrite(ciphered, 1, tam, fpout));
	fclose(fpout);
	fclose(fp);
	return 0;
}

void mutant_caesar_menu() {
	BOOL exit = FALSE;
	int choice;
	system("cls");
	while (!exit) {
		printf("\n");
		printf("Select a mode:\n");
		printf("  1) Demo wit user input\n");
		printf("  2) Cipher Quixote\n");
		printf("  3) Cipher Quixote with Rc4\n");
		printf("  4) TODO Cipher Quixote with AES\n");
		printf("  5) TODO Cipher Quixote with Cesar-1\n");
		printf("  0) Exit\n");
		choice = atoi(inputString(stdin, 1));
		switch (choice) {
		case 1:
			mutant_caesar_64();
			break;
		case 2:
			mutant_caesar_file();
			break;
		case 3:
			cipher_rc4();
			break;
		case 0:
			printf("Good bye!\n");
			exit = TRUE;
			break;
		default:
			printf("Invalid, please enter a valid option. Actual choice=%d\n", choice);
			break;
		}
	}
}
#pragma endregion

#pragma region Mutant caesar with MD5

void mutant_cipher_md5(char plain, char* ciphered, char* key, int pos) {
	//PRINT2("Entrada: char= %c, clave= %s, pos= %d\n",plain, key, pos);
	byte* result;
	char concat[6];
	concat[0] = (char)pos;
	concat[1] = key[0];
	concat[2] = key[1];
	concat[3] = key[2];
	concat[4] = key[3];
	concat[5] = NULL;
	//PRINT2("Concat %c, %s = %s\n", plain, key,concat);
	result = md5String(concat);
	//PRINT2("");
	//print_hash(result);
	int total = 0;
	for (int j = 0; j < 16; j++) {
		total += result[j];
	}
	total = total % 255;
	//PRINT2("Suma del hash: %d", total);
	total = ((int)plain + total)%256;
	//PRINT2("Suma del hash con el caracter en plano: %d, en char: %c\n", total, (byte)total);
	((char*)ciphered)[pos] = (byte)total;
	//ciphered[pos] = NULL; // (char)total;
	//PRINT2("Los resultados son: suma=%d, final = %02X\n",  total, ((byte*)ciphered)[pos]);
	free(result);
}

void mutant_cipher_md5_buff(char* plain, char* ciphered, char* key, int tam) {
	//PRINT2("Entrada: char= %c, clave= %s, pos= %d\n",plain, key, pos);
	byte* result;
	char concat[12];
	int total;
	for (int i = 0; i < tam; i++) {
		/*concat[0] = (char)i;
		concat[1] = key[0];
		concat[2] = key[1];
		concat[3] = key[2];
		concat[4] = key[3];
		concat[5] = NULL;*/
		//Math.Floor(Math.Log10(n) + 1);
		snprintf(concat, 12, "%d%s", i, key);
		//PRINT2("%s\n", concat);

		result = md5String(concat);
		total = 0;
		for (int j = 0; j < 16; j++) {
			total += result[j];
		}
		//total = total % 256;

		total = ((int)plain[i] + total) % 256;
		((char*)ciphered)[i] = (byte)total;
		free(result);
	}
}

void mutant_decipher_md5(char ciphered, char* deciphered, char* key, int pos) {
	//PRINT2("Entrada: char= %c, clave= %s, pos= %d\n", ciphered, key, pos);
	byte* result;
	char concat[6];
	concat[0] = (char)pos;
	concat[1] = key[0];
	concat[2] = key[1];
	concat[3] = key[2];
	concat[4] = key[3];
	concat[5] = NULL;
	//PRINT2("Concat %c, %s = %s\n", ciphered, key, concat);//strcat(plain, key)); //OJO FALLA EL CONCAT PORQUE LE MANDO UN CHAR Y NO UN STRING ver chrome , 
	//esta desrecomendado, hacer un array de tamalo key + 1 y meter todo.
	result = md5String(concat);
	//PRINT2("");
	//print_hash(result);
	int total = 0;
	for (int j = 0; j < 16; j++) {
		total += result[j];
	}
	total = total % 255;
	//PRINT2("Suma del hash: %d", total);
	total = ((int)ciphered - total) % 256;
	//PRINT2("Suma del hash con el caracter en plano: %d, en char: %c\n", total, (byte)total);
	((char*)deciphered)[pos] = (byte)total;
	//ciphered[pos] = NULL; // (char)total;
	//PRINT2("Los resultados son: suma=%d, final = %02X\n",  total, ((byte*)ciphered)[pos]);
	free(result);
}

void mutant_decipher_md5_buff(char* ciphered, char* deciphered, char* key, int tam) {
	//PRINT2("Entrada: char= %c, clave= %s, pos= %d\n",plain, key, pos);
	byte* result;
	char concat[12];
	int total;
	for (int i = 0; i < tam; i++) {
		/*concat[0] = (char)i;
		concat[1] = key[0];
		concat[2] = key[1];
		concat[3] = key[2];
		concat[4] = key[3];
		concat[5] = NULL;*/
		snprintf(concat, 12, "%d%s", i, key);
		//PRINT2("%s\n", concat);
		result = md5String(concat);
		total = 0;
		for (int j = 0; j < 16; j++) {
			total += result[j];
		}
		//total = total % 256;

		total = ((int)ciphered[i] - total) % 256;
		((char*)deciphered)[i] = (byte)total;
		free(result);
	}
}


void md5_mutant() {
	PRINT("Cesar mutante con MD5\n");
	FILE* fp = NULL;
	uint8_t* result;
	char* message;
	int tam = 0;
	char* key = "hola";
	//key = "JfbzxCM3qTjeAuk5Bb5pLDmgNymLeBh7piyh2U9tPaXwvzD6WPBz6YAUpVWSdkb44QZ72MxGgSH ";

	fp = fopen("el_quijote.txt", "r");
	FILE* fpout = NULL;
	fpout = fopen("fichero_cifradomd5.txt", "w");
	FILE* fpout2 = NULL;
	fpout2 = fopen("fichero_descifradomd5.txt", "w");
	//result = md5File(fp);
	//result = md5String("olakase");
	//print_hash(result);
	//message = "hasta luego";//inputFile(fp, 1);
	message = inputFile(fp, 1);
	tam = strlen(message);
	PRINT("A cifrar:\n");
	//char* experimento = "1234567hola"; //Para ejecutar solamente md5 n veces y ver cuanto tarda
	clock_t start = clock();
	/*for (int j = 0; j < tam; j++) {
		md5String(experimento);
	}
	clock_t end = clock();*/
	char* ciphered = malloc(tam*sizeof(char));
	mutant_cipher_md5_buff(message, ciphered, key, tam);
	/*for (int i = 0; i < tam; i++) {
		mutant_cipher_md5(message[i], ciphered, key, i);
	}*/
	clock_t end = clock();
	char* deciphered = malloc(tam * sizeof(char));
	PRINT("A descifrar:\n");
	mutant_decipher_md5_buff(ciphered, deciphered, key, tam);
	/*for (int i = 0; i < tam; i++) {
		mutant_decipher_md5(ciphered[i], deciphered, key, i);
	}*/
	fwrite(ciphered, 1, tam, fpout);
	fwrite(deciphered, 1, tam, fpout2);
	/*PRINT("Mensaje:");
	PRINT_HEX(message, tam );
	PRINT("\nCifrado:");
	PRINT_HEX(ciphered, tam );
	PRINT("\nDescifrado:");
	PRINT_HEX(deciphered, tam);
	PRINT("%s\n",deciphered);*/

	//cipher_md5(message, ciphered, tam);
	//free(result);
	free(ciphered);
	free(deciphered);
	fclose(fp);
	float seconds = (float)(end - start) / CLOCKS_PER_SEC;
	PRINT("He tardado en cifrar %.16f segundos\n", seconds);
}
#pragma endregion

void main() {
	BOOL exit = FALSE;
	int choice;
	system("cls");
	printf("  __  __       _              _          _       _               \n");
	printf(" |  \\/  |     | |            | |        (_)     | |              \n");
	printf(" | \\  / |_   _| |_ __ _ _ __ | |_    ___ _ _ __ | |__   ___ _ __ \n");
	printf(" | |\\/| | | | | __/ _` | '_ \\| __|  / __| | '_ \\| '_ \\ / _ \\ '__|\n");
	printf(" | |  | | |_| | || (_| | | | | |_  | (__| | |_) | | | |  __/ |   \n");
	printf(" |_|  |_|\\__,_|\\__\\__,_|_| |_|\\__|  \\___|_| .__/|_| |_|\\___|_|   \n");
	printf("                                          | |                    \n");
	printf("                                          |_|                    \n");
	while (!exit) {
		printf("\n");
		printf("Welcome! Select a cipher:\n");
		printf("  1) Caesar dummy\n");
		printf("  2) Mutant Caesar with 16bytes Matrix\n");
		printf("  3) Mutant Caesar with improvements\n");
		printf("  4) Mutant Caesar with searching\n");
		printf("  5) Mutant Caesar with ultimate version\n");
		printf("  6) Mutant Caesar with MD5 function\n");
		printf("  0) Exit\n");
		choice = atoi(inputString(stdin, 1));
		switch (choice) {
			case 1:
				dummy_caesar();
				break;
			case 2:
				mutant_caesar();
				break;
			case 3:
				mutant_caesar_develop();//Queda hacer la matriz mutable
				break;
			case 4:
				mutant_caesar_search();//queda hacer que genere directamente la matriz numero n, ESTA EMPEZADO, PERO NO ESTA BIEN
				break;
			case 5:
				mutant_caesar_menu();
				break;
			case 6:
				md5_mutant(); 
				break;
			case 0:
				printf("Good bye!\n");
				exit = TRUE;
				break;
			default:
				printf("Invalid, please enter a valid option. Actual choice=%d\n",choice);
				break;
		}
	}
}


/*clock_t start = clock(); //con time.h
clock_t end = clock();
float seconds = (float)(end - start) / CLOCKS_PER_SEC; */