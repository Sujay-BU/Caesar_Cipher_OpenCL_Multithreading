__kernel void caesarCipher(__constant char* input, __global char* result, __private const int shift, __private const unsigned int count) {
	int i = get_global_id(0);
	if (i < (count - 1)) {
		char c = input[i];
		((c>='a' && c<='z') ? (c = (char)(((int)input[i]+shift-97)%26+97)) : (c>='A' && c<='Z') ? (c = (char)(((int)input[i]+shift-65)%26+65)) : NULL); //Performs the shift even if the key is larger than 26 based on if the character passed if lowercase or uppercase.
		result[i] = c;
	}
};

__kernel void caesarCipherOne(__constant char* input, __global char* result, __private const int shift, __private const unsigned int count, __private const unsigned int new_count) {
	int i = get_global_id(0);
	if (i < (count - 1) && i >= new_count) {
		char c = input[i];
		((c>='a' && c<='z') ? (c = (char)(((int)input[i]+shift-97)%26+97)) : (c>='A' && c<='Z') ? (c = (char)(((int)input[i]+shift-65)%26+65)) : NULL); //Performs the shift even if the key is larger than 26 based on if the character passed if lowercase or uppercase.
		result[i-new_count] = c;
	}
};