Archivo MakeFile make comppila make clean borra el archivo test.img creado por la ejecución de write_gpt
#funciona de la siguente forma
#write_gp.c tiene hardcodeado la inclusion de un archivo llamado BOOTX64.EFI
#cuando inicia Qemu con OVMF.fd automaticamente busca el archivo con ese nombre y lo ejecuta
#nuestros archivos de prueba seran creado con ese nombre para poder verificar su correcto funcionamiento
