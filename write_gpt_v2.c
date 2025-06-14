#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <uchar.h> //u""
//i----------------------------------
//Globarl Typedef
//i----------------------------------
//Globally Unique identifier (aka UUID) 
typedef struct{
	uint32_t 	time_lo;
	uint16_t	time_mid;
	uint16_t	time_hi_and_ver; 	//higest 4 bits are version #
	uint8_t		clock_seq_hi_and_res;	//Hihest bits are varian #				 
	uint8_t		clock_seq_lo;		
	uint8_t		node[6];	
}__attribute__((packed)) Guid;
typedef struct{	
	uint8_t boot_indicator;
	uint8_t starting_chs[3];
	uint8_t os_type;
	uint8_t ending_chs[3];
	uint32_t starting_lba;
	uint32_t size_lba;
} __attribute__((packed)) Mbr_Partition;
	
//Master Boot Record
typedef struct{
	uint8_t boot_code[440];
	uint32_t mbr_signature;
	uint16_t unknown;
	Mbr_Partition partition [4];
	uint16_t boot_signature;
} __attribute__((packed)) Mbr;
//GPT header
typedef struct{
	uint8_t 	signature[8];
	uint32_t 	revision;
	uint32_t	header_size;
	uint32_t	header_crc32;
	uint32_t	reserved_1;
	uint64_t	my_lba;
	uint64_t	alternate_lba;
	uint64_t	first_usable_lba;
	uint64_t	last_usable_lba;
	Guid  disk_guid;	//GUID spceial number
	uint64_t	partition_table_lba;
	uint32_t	number_of_entries;
	uint32_t	size_of_entry;
	uint32_t	partition_table_crc32;

	uint8_t		reserved_2[512-92];
}__attribute__((packed)) Gpt_Header;

//GPT Partition Entrry
typedef struct {
	Guid partition_type_guid;
	Guid unique_guid;
	uint64_t starting_lba;
	uint64_t ending_lba;
	uint64_t attributes;
	char16_t name[36]; //UCS-2 (UUTF-16 LIMITED TO CODE POINTS 0X0000 - 0Xffff)
}__attribute__((packed)) Gpt_Partition_Entry;
//i----------------------------------
//Globa constanst, enumbk 
//i----------------------------------
const Guid ESP_GUID = {  0XC12A7328, 0XF81F, 0X11D2, 0XBA, 0x4B, 
			{0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}};
const Guid BASIC_DATA_GUID = {  0xEBD0A0A2, 0xB9E5, 0x4433, 0x87, 0xC0, 
				{0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}};

enum {
	GPT_TABLE_ENTRY_SIZE = 128,
	NUMBER_OF_GPT_TABLE_ENTRIES = 128, 
	GPT_TABLE_SIZE = 16384,			//minimu size for UEFI
	ALIGNMENT = 1048576,			//1 MiB alignament value						
};
//i----------------------------------
//Global VariablesL 
//i----------------------------------
//Master Boot Record
char *image_name = "test.img";
uint64_t lba_size = 512;
uint64_t esp_size = 1024*1024*33;	// 33 MiB
uint64_t data_size = 1024*1024*1;	// 1 MiB
uint64_t image_size = 0;
uint64_t esp_size_lbas = 0, data_size_lbas = 0, image_size_lbas = 0; //Sizes in LBA
uint64_t align_lba = 0, esp_lba = 0, data_lba = 0; //Starting lba value of
//==================================
//convert bytes to LBAs
//===================================
uint64_t bytes_to_lbas(const uint64_t bytes){
	return (bytes / lba_size) + (bytes % lba_size > 0 ? 1 :0);
}
//================================
//Pad out 0s to full lba size
//================================
void write_full_lba_size(FILE *image){
	uint8_t	zero_sector[512];
	for (uint8_t i = 0; i < (lba_size - sizeof zero_sector) / sizeof zero_sector; i++)
		fwrite(zero_sector, sizeof zero_sector, 1, image);
}
//================================
//Get next highest aligned lba value after input lba
//================================
uint64_t next_aligned_lba(const uint64_t lba){
	return	lba - (lba % align_lba) + align_lba;
}
//================================
//Create a new Version 4 Varian 2 GUID
//================================
Guid new_guid (void){
	uint8_t rand_arr[16] = { 0 };

	for (uint8_t i = 0; i < sizeof rand_arr; i++)
		rand_arr[i] = rand() % (UINT8_MAX + 1);
	
	//TODO: fill out GUID
	Guid result = {
		.time_lo 	 = *(uint32_t *)&rand_arr[0],
		.time_mid 	 = *(uint16_t *)&rand_arr[4],
		.time_hi_and_ver = *(uint16_t *)&rand_arr[6],
		.clock_seq_hi_and_res = rand_arr[7],
		.clock_seq_lo = rand_arr[8],
		.node = { rand_arr[10], rand_arr[11], rand_arr[12], rand_arr[13], 
			  rand_arr[14], rand_arr[15] },			
	};
	//fill out version bits
	result.time_hi_and_ver |= ~(1 << 15);  //0b_0_111 1100			 
	result.time_hi_and_ver |= (1 << 14);    //0b__1_00 0000
	result.time_hi_and_ver &= ~(1 << 13);   //ob11_0_1 1111 
	result.time_hi_and_ver &= ~(1 << 12);   //ob111_0_ 1111 
	// fill out variant bits
	result.clock_seq_hi_and_res |= (1 << 7);	// 0b_1_000 0000
	result.clock_seq_hi_and_res |= (1 << 6);	// 0b0_1_00 0000
	result.clock_seq_hi_and_res &= ~(1 << 5);	// 0b11_0_1 1111
	
	return result;
}

//================================
// Create CRC32 rVLW VALUES 
// ===============================
uint32_t crc_table[256];

void create_crc32_table(void) {
	uint32_t c = 0;

	for (uint32_t n = 0; n < 256; n++) {
       		c = (uint32_t) n;
       		for (uint8_t k = 0; k < 8; k++) {
         		if (c & 1)
		   	     c = 0xedb88320L ^ (c >> 1);
         		else
		           c = c >> 1;
	       }	
       		crc_table[n] = c;
	}
}
//================================
// Create CRC32 value for range of data
// ===============================
uint32_t calculate_crc32(void *buf, int32_t len) {
	static bool made_crc_table = false;

	uint8_t *bufp = buf;
	uint32_t c = 0xFFFFFFFFL;

	if (!made_crc_table){
		create_crc32_table();
		made_crc_table = true;
	}
	for (int32_t n=0; n < len; n++)
		c = crc_table[(c^bufp[n]) & 0xFF]^(c >> 8);
	//Invert bits from return value
	return c^0xFFFFFFFFL;
}


//================================
// Write Protective MBR
// ===============================
bool write_mbr(FILE *image){
	uint64_t mbr_image_lbas = image_size_lbas;
	if (mbr_image_lbas > 0xFFFFFFFF) mbr_image_lbas = 0x100000000;
	Mbr mbr = {
		.boot_code = { 0 },
		.mbr_signature = 0,
		.unknown = 0,
		.partition[0] = {
			.boot_indicator = 0,
			.starting_chs = { 0x00, 0x02, 0x00 },
			.os_type = 0xEE,	//Protective GPT
			.ending_chs = {0xFF, 0xFF, 0xFF},
			.starting_lba = 0x00000001,
			.size_lba = mbr_image_lbas - 1,
		},
		.boot_signature = 0xAA55,
	};

	//Write to file
	if (fwrite(&mbr, 1, sizeof mbr, image) != sizeof mbr)
		return false;

	write_full_lba_size(image);
	return true;
}
//=====================================
//Write GPT headers primary & secondary
//====================================
bool write_gpts(FILE *image){
	//Fill out primary GPT header
	Gpt_Header primary_gpt = {
		.signature = {"EFI PART"},
		.revision = 0x00010000, //version 1.0
		.header_size = 92,
		.header_crc32 = 0,	//Will calculate later
		.reserved_1 = 0,
		.my_lba = 1,		//lba 1 is right after MBR
		.alternate_lba = image_size_lbas - 1,
		.first_usable_lba = 1 + 1 +32, //mbr + gpt + Primayr gpt table
		.last_usable_lba = image_size_lbas -1 -32 -1, //2nd GPT header, table
		.disk_guid = new_guid(), 
		.partition_table_lba = 2, //After MBR + GPT header
		.number_of_entries = 128,
		.size_of_entry = 128,
		.partition_table_crc32 = 0, //Will calculate later
		.reserved_2 = { 0 },    
	};

	
	//TODO: MAKE FUNCTION TO CALCULATE crc32 VALUES
	Gpt_Partition_Entry gpt_table[NUMBER_OF_GPT_TABLE_ENTRIES] = {
		//efi sYSTEM PARTITION
		{
			.partition_type_guid = ESP_GUID,
			.unique_guid = new_guid(),
			.starting_lba = esp_lba,
			.ending_lba = esp_lba + esp_size_lbas,
			.attributes = 0,
			.name = u"EFI SYSTEM",
		},
		//Basic Data partition
		{
			.partition_type_guid = BASIC_DATA_GUID,
			.unique_guid = new_guid(),
			.starting_lba = data_lba,
			.ending_lba = data_lba + data_size_lbas,
			.name = u"BASIC DATA",
		}

	};
	//tODO : fILL OUT PRIMARY  TABLE PARTION ENTRIES
	primary_gpt.partition_table_crc32 = calculate_crc32(gpt_table, sizeof gpt_table);
	primary_gpt.header_crc32 = calculate_crc32(&primary_gpt, primary_gpt.header_size);	
	//todo: fIRR OUT PRIMARY crc VALUES
	if (fwrite(&primary_gpt, 1, sizeof primary_gpt, image) != sizeof primary_gpt)
		return false;
	write_full_lba_size(image);
	//todo: Write primary gpt header to fiel
	if (fwrite(&gpt_table, 1, sizeof gpt_table , image) != sizeof gpt_table)
		return false;

	//Fill out secondary GPT header
	Gpt_Header secondary_gpt = primary_gpt;

	secondary_gpt.header_crc32 = 0;
	secondary_gpt.partition_table_crc32 =0;
	secondary_gpt.my_lba = primary_gpt.alternate_lba;
	secondary_gpt.alternate_lba = primary_gpt.my_lba;
	secondary_gpt.partition_table_lba = image_size_lbas - 1 -32;

	//FFill out secondary  CRC values
	secondary_gpt.partition_table_crc32 = calculate_crc32(gpt_table, sizeof gpt_table);
	secondary_gpt.header_crc32 = calculate_crc32(&primary_gpt, primary_gpt.header_size);

	//Goto to position of secondary table
	fseek(image, secondary_gpt.partition_table_lba * lba_size ,SEEK_SET);
	//Write  secondary gpt table of the file
	if (fwrite(&gpt_table, 1, sizeof gpt_table , image) != sizeof gpt_table)
		return false;
	//Write secondary gpt header to file
	if (fwrite(&secondary_gpt, 1, sizeof secondary_gpt , image) != sizeof secondary_gpt)
		return false;
	write_full_lba_size(image);	

	return true;
}

//=====================================
//MAIN
//====================================
int main(void){	
	FILE *image = fopen(image_name, "wb+");
	if (!image){
		fprintf(stderr, "Error: coud not open file %s\n", image_name);
		return EXIT_FAILURE;
	}
	//Set sizes & lbas
	const uint64_t padding = (ALIGNMENT *2 + (lba_size *67)); //extra padding for GPTs/MBR
	image_size = esp_size + data_size + padding; // add some extra padding for GPTs /MBR
	image_size_lbas = bytes_to_lbas(image_size);					
	align_lba = ALIGNMENT / lba_size;
	esp_lba = align_lba;
	esp_size_lbas = bytes_to_lbas(esp_size);
	data_size_lbas = bytes_to_lbas(data_size);
	data_lba = next_aligned_lba(esp_lba + esp_size_lbas);

	// Seed randoom number generation
	srand(time(NULL));
	
		
	//Write pritecive MBR
	if (!write_mbr(image)){
		fprintf(stderr, "Error: coud not protective MBR for file %s\n", image_name);
		return EXIT_FAILURE;
	}

	//write GPT headers & tables
	if (!write_gpts(image)){
		fprintf(stderr, "Error: coud not write GPT headers & tables for file %s\n", image_name);
		return EXIT_FAILURE;
	}	

	return EXIT_SUCCESS;
}

