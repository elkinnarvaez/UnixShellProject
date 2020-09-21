compile:
	gcc main.c -o main.out

shell:
	gcc material_adicional/shell.c -o shell.out

test:
	gcc material_adicional/test.c -o material_adicional/test.out
    
change_directory:
	gcc material_adicional/change_directory.c -o material_adicional/change_directory.out
	
clean:
	rm -f *.out
    