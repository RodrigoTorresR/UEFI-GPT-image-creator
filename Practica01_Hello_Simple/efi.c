//
#include "efi.h"

//entry point set up
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable){
	//TODO: Remove this line when using input params 
	(void)ImageHandle, (void)SystemTable; //prevent compiler warnings
	
	// Reset Console Output
	SystemTable->ConOut->Reset(SystemTable->ConOut, false);
	
	//CLEAR console outpout (clear screen to background color and
	// set cursor to 0.0
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut); 
	
	// Write String
	SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Hola Acha\r\n");
	
	// Infinite Loop
	while (1);
	
	return EFI_SUCCESS;
}
//int main()
//{
//...
//  return 0
//}
