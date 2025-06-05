#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <uchar.h> //u""
#include <string.h>
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
} __attribute__ ((packed)) Guid;
typedef struct{	
	uint8_t boot_indicator;
	uint8_t starting_chs[3];
	uint8_t os_type;
	uint8_t ending_chs[3];
	uint32_t starting_lba;
	uint32_t size_lba;
} __attribute__ ((packed)) Mbr_Partition;
	
//Master Boot Record
typedef struct{
	uint8_t boot_code[440];
	uint32_t mbr_signature;
	uint16_t unknown;
	Mbr_Partition partition [4];
	uint16_t boot_signature;
} __attribute__ ((packed)) Mbr;
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
} __attribute__ ((packed)) Gpt_Header;

//GPT Partition Entrry
typedef struct {
	Guid partition_type_guid;
	Guid unique_guid;
	uint64_t starting_lba;
	uint64_t ending_lba;
	uint64_t attributes;
	char16_t name[36]; //UCS-2 (UUTF-16 LIMITED TO CODE POINTS 0X0000 - 0Xffff)
} __attribute__ ((packed)) Gpt_Partition_Entry;
//FAT32 Volume boot record (VBR)
typedef struct{
	uint8_t	BS_jmpBoot[3];
	uint8_t	BS_OEMName[8];
	uint16_t BPB_BytesPerSec;
	uint8_t	 BPB_SecPerclus;
	uint16_t BPB_RsvdSecCnt;
	uint8_t BPB_NumFATs;
	uint16_t BPB_RootEntCnt;
	uint16_t BPB_TotSec16;
	uint8_t BPB_Media;
	uint16_t BPB_FATSz16;
	uint16_t BPB_SecPerTrk;
	uint16_t BPB_NumHeads;
	uint32_t BPB_HiddSec;
	uint32_t BPB_TotSec32;
	uint32_t BPB_FATSz32;
	uint16_t BPB_ExtFlags;
	uint16_t BPB_FSVer;
	uint32_t BPB_RootClus;
	uint16_t BPB_FSInfo;
	uint16_t BPB_BkBootSec;
	uint8_t  BPB_Reserved[12];
	uint8_t  BS_DrvNum;
	uint8_t  BS_Reserved1;
	uint8_t  BS_BootSig;
	uint8_t  BS_VolID[4];
	uint8_t  BS_VolLab[11];
	uint8_t  BS_FilSysType[8];
	//Not in fatgen103.doc tables
	uint8_t	 boot_code[510-90];
	uint16_t bootsec_sig;	//0xAS55
} __attribute__ ((packed)) Vbr;
//FAT32 file system Info Sector
typedef struct{
	uint32_t FSI_LeadSig;
	uint8_t  FSI_Reserved1[480];
	uint32_t FSI_StructSig;
	uint32_t FSI_Free_Count;
	uint32_t FSI_Next_Free;
	uint8_t  FSI_Reserved2[212];
	uint32_t FSI_TrailSig;
} __attribute__ ((packed)) FSInfo;
//FAT32 Directory Entry (Short Name)
 typedef struct {
	 uint8_t DIR_Name[11];
	 uint8_t DIR_Attr;
	 uint8_t DIR_NTRes;
	 uint8_t DIR_CrtTimeTenth;
	 uint16_t DIR_CrtTime;
	 uint16_t DIR_CrtDate;
	 uint16_t DIR_LstAccDate;
	 uint16_t DIR_FstClusHI;
	 uint16_t DIR_WrtTime;
	 uint16_t DIR_WrtDate;
	 uint16_t DIR_FstClusL0;
	 uint32_t DIR_FileSize;
 } __attribute__ ((packed)) FAT32_Dir_Entry_Short;

//FAT32 Directroy Entry Attributes
typedef enum {
	ATTR_READ_ONLY	= 0X01,
	ATTR_HIDDEN	= 0X02,
	ATTR_SYSTEM	= 0X04,
	ATTR_VOLUME_ID	= 0X08,
	ATTR_DIRECTORY 	= 0X10,
	ATTR_ARCHIVE	= 0x20,
	ATRR_LONG_NAME  = ATTR_READ_ONLY | ATTR_HIDDEN |
			  ATTR_SYSTEM	 | ATTR_VOLUME_ID,
}FAT3_Dir_Atrr;
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
uint64_t esp_size_lbas = 0, data_size_lbas = 0, image_size_lbas = 0,  //Sizes in LBA
	gpt_table_lbas = 0;								      //
uint64_t align_lba = 0, esp_lba = 0, data_lba = 0, //Starting lba value of
	fat32_fats_lba = 0, fat32_data_lba = 0;				
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
//Get new dat/time values for FAT32 directory entries
// ===============================
void get_fat_dir_entry_time_date(uint16_t *in_time, uint16_t *in_date){
	time_t curr_time;
	curr_time = time(NULL);
	struct tm tm = *localtime(time(NULL));
	
	//Fat32 needs # of years since 1980, localtime return tm_year as # years since 1900
	//subtra 80 years for correct year value.Also convert month of year from 0-11 to 1 -12
	//by adding 1
	*in_time = ((tm.tm_year - 80) << 9) | ((tm.tm_mon + 1) << 5) | tm.tm_mday;
	// Seconds is # 2-sexond count, 0-29		
	*in_date = tm.tm_hour << 11 | tm.tm_min << 5 | (tm.tm_sec / 2);
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
		.first_usable_lba = 1 + 1 + gpt_table_lbas, //mbr + gpt + Primayr gpt table
		.last_usable_lba = image_size_lbas -1 -gpt_table_lbas - 1, //2nd GPT header, table
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
	secondary_gpt.partition_table_lba = image_size_lbas -1 - gpt_table_lbas;

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
//Write GPT system parttition (ESP) /with FAT32 filesystem
//====================================
bool write_esp(FILE *image){
	//Reserver sector region  ..................
	//TODO: Fillout Volume Boot Record (VBR)
	const uint8_t reserved_sectors = 32; //FAT32
	Vbr vbr = {
		.BS_jmpBoot = { 0xEB, 0x00, 0x90},
 		.BS_OEMName = { "THIS DISK" },
 		.BPB_BytesPerSec = lba_size, //This is limited to only 512/1024/2048/4096
	 	.BPB_SecPerclus = 1,
 		.BPB_RsvdSecCnt = reserved_sectors,
		.BPB_NumFATs = 2,
		.BPB_RootEntCnt = 0,
		.BPB_TotSec16 = 0,
		.BPB_Media = 0xF8, //"Fixed" non -removable media; Couould also be 0xF00 for e.g. flash drive
		.BPB_FATSz16 = 0,
		.BPB_SecPerTrk = 0,
		.BPB_NumHeads = 0,
		.BPB_HiddSec = esp_lba - 1, //# of sectors before this partition/voume
		.BPB_TotSec32 = esp_size_lbas, //Size of this partition
		.BPB_FATSz32 = (align_lba - reserved_sectors) / 2,
		.BPB_ExtFlags = 0,	//Mirroed FATs
		.BPB_FSVer = 0,
		.BPB_RootClus = 2,	//Cluster 0 & 1 are reserved; root dir cluster starts at 2
		.BPB_FSInfo =1,		//Sector 0 = this VBR; FS Info sector follwsd it
		.BPB_BkBootSec = 6,
		.BPB_Reserved = { 0 },	
		.BS_DrvNum = 0x80,	//First hard drive
		.BS_Reserved1 = 0,
		.BS_BootSig = 0x29, 
		.BS_VolID = { 0 },
		.BS_VolLab = { "NO NAME   " }, //no bolume label
		.BS_FilSysType = { "FAT32    "}, //k
		//Not in fatgen103.doc tables
	 	.boot_code = { 0 },
		.bootsec_sig = 0xAA55, 	//0xAS55
	};	
	//TODO: Fill out file system info sector
	FSInfo fsinfo= {
		.FSI_LeadSig = 0x41615252,
		.FSI_Reserved1 = { 0 },
		.FSI_StructSig = 0x61417272,
		.FSI_Free_Count = 0xFFFFFFFF,
		.FSI_Next_Free = 0xFFFFFFFF,
		.FSI_Reserved2 = { 0 },
		.FSI_TrailSig = 0xAA550000,
	};
		
	//T write VVR and FSInfo
	fseek(image, esp_lba * lba_size, SEEK_SET);
	if (fwrite(&vbr, 1, sizeof vbr, image) != sizeof vbr){
		fprintf(stderr, "Error: Could not write ESP VBR to image\n");
		return false;
	}
	write_full_lba_size(image);

	if (fwrite(&fsinfo, 1, sizeof fsinfo, image) != sizeof fsinfo){
		fprintf(stderr, "Error: Could not write ESP File System Info Sector to image\n");
		return false;
	}
	write_full_lba_size(image);

	//Go to  back up boot sector ocation
	fseek(image, (esp_lba + vbr.BPB_BkBootSec) * lba_size, SEEK_SET);	
	
	//TODO: Write VBR and FSInf and backu  locationo
	fseek(image, esp_lba * lba_size, SEEK_SET);
	if (fwrite(&vbr, 1, sizeof vbr, image) != sizeof vbr){
		fprintf(stderr, "Error: Could not write VBR to image\n");
		return false;
	}
	write_full_lba_size(image);

	if (fwrite(&fsinfo, 1, sizeof fsinfo, image) != sizeof fsinfo){
		fprintf(stderr, "Error: Could not write ESP File System Info Sector to image\n");
		return false;
	}
	write_full_lba_size(image);
	
	//FAT region ----------------------
	//TODO:Write FATsi (NOTE FATs will be mirrored)
	//fseek(image, (esp_lba + vbr.BPB_RsvdSecCnt) * lba_size , SEEK_SET)
	const uint32_t fat_lba = esp_lba + vbr.BPB_RsvdSecCnt;
	for (uint8_t i = 0; i < vbr.BPB_NumFATs; i++) {
		fseek(image,
			(fat_lba + (i*vbr.BPB_FATSz32)) * lba_size, 
			SEEK_SET);
		
		uint32_t cluster = 0;

		// /efi/boot
		//Cluster 0: FAT identifier, lowest 8 bits are the media type/byte
		cluster = 0xFFFFFF00 | vbr.BPB_Media;
		fwrite(&cluster, sizeof cluster, 1, image);

		//Cluset 1; End of chain (EOC) marker
		cluster = 0xFFFFFFFF;
		fwrite(&cluster, sizeof cluster, 1, image);

		//Cluster 2; Root dir '/' cluster star, if end of file/dir data then write EOC marker
		cluster = 0xFFFFFFFF;
		fwrite(&cluster, sizeof cluster, 1, image);

		//Cluster 3; u ' /EFI DIR CLUSTER
		cluster = 0xFFFFFFFF;
		fwrite(&cluster, sizeof cluster, 1, image);

		//Cluster 4; u ' /EFI/BOOT
		cluster = 0xFFFFFFFF;
		fwrite(&cluster, sizeof cluster, 1, image);

		//Cluster 5+; Other files/directories...
		// e. g. if adding a file with a size = 5 sectors/clusters
		// cluster = 6;	//Point to next cluster containing file data	
		// cluster = 7;	//Point to next cluster containing file data	
		// cluster = 8;	//Point to next cluster containing file data	
		// cluster = 9;	//Point to next cluster containing file data	
		// cluster = 0xFFFFFFFF;;	ECOC marker, no more file data after this custer	
	}	
	//Dataregion ...........................
	//Write File/dir data...................i
	const uint32_t data_lba = fat_lba + (vbr.BPB_NumFATs * vbr.BPB_FATSz32);
	fseek(image, data_lba * lba_size, SEEK_SET);

	//TODO: Root ''/' Director
	//"/EFI" dir entryy
	FAT32_Dir_Entry_Short dir_ent = {
		.DIR_Name = { "EFI      " },
		.DIR_Attr = ATTR_DIRECTORY,
		.DIR_NTRes = 0,
		.DIR_CrtTimeTenth = 0,
		.DIR_CrtDate = 0,
		.DIR_LstAccDate = 0,
		.DIR_FstClusHI = 0,
		.DIR_WrtTime = 0,
		.DIR_WrtDate = 0,
		.DIR_FstClusL0 = 3,
		.DIR_FileSize = 0, //Directories have 0 file size
	};	

	uint16_t create_time = 0, create_date = 0;
	get_fat_dir_entry_time_date(&create_time, &create_date);

	dir_ent.DIR_CrtTime = create_time;
	dir_ent.DIR_CrtDate = create_date;
	dir_ent.DIR_WrtTime = create_time;
	dir_ent.DIR_WrtDate = create_date;
	
	fwrite(&dir_ent, sizeof dir_ent, 1, image);
	///EFI Directory entriesy
	fseek(image, (data_lba + 1) * lba_size, SEEK_SET);
	
	memcpy(dir_ent.DIR_Name, ".           ", 11); //"." dir entry, this drectrory itself
	fwrite(&dir_ent, sizeof dir_ent, 1, image);

	memcpy(dir_ent.DIR_Name, "..          ", 11); //"." dir entry, this is parent dir (ROOT dir)
	dir_ent.DIR_FstClusL0 = 0;		//Root directory does not have a cluster value
	fwrite(&dir_ent, sizeof dir_ent, 1, image);

	memcpy(dir_ent.DIR_Name, "BOOT       ", 11); //EI/BOOT boot directory
	dir_ent.DIR_FstClusL0 = 4;		//EFI/BOOT/ cluster:w
	fwrite(&dir_ent, sizeof dir_ent, 1, image);

	///EFI/BOOT Directories entries
	fseek(image, (data_lba + 2) * lba_size, SEEK_SET);
	
	memcpy(dir_ent.DIR_Name, ".           ", 11); //"." dir entry, this drectrory itself
	fwrite(&dir_ent, sizeof dir_ent, 1, image);

	memcpy(dir_ent.DIR_Name, "..          ", 11); //"." dir entry, parent dir (/EFI dir)
	dir_ent.DIR_FstClusL0 = 3;		// /EFI directory, parent dire
	fwrite(&dir_ent, sizeof dir_ent, 1, image);
	

	return true;
}
//=====================================
//Add a file path to eh EFI System
//wnwe file at end of path
//====================================
add_path_to_esp(path, image){
	//TODO:
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
	FILE *fp = NULL;
	//Set sizes & lbas
	gpt_table_lbas = GPT_TABLE_SIZE / lba_size;

	const uint64_t padding = (ALIGNMENT *2 + (lba_size * ((gpt_table_lbas * 2) + 1 + 2)) ); //extra padding for GPTs/MBR
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
	//write EFI SYSTEM PARTITION w/Fat32 filesystem
	if (!write_esp(image)){
		fprintf(stderr, "Error: coud not write ESP for file %s\n", image_name);
		return EXIT_FAILURE;
	}	
	
	//Check if ""BOOTX64.EFI" file exist in currrent directory if so atutomatically add
	Fp = fopen("BOOTX64.EFI", "rb");
	if (fp) {
		fclose(fp);
		char *path[25]= calloc(1, 25);
		strcpy (path, "/EFI/BOOT/BOOTIX64.EFI");
		if (!add_path_to_esp(path,image)){
			fprintf(stderr, "Error: Could not add file '%s'\n", path);

		}	
	}
	
	return EXIT_SUCCESS;
}

