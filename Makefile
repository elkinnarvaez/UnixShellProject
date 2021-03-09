compile:
	gcc main.c -o main.out

test:
	gcc material_adicional/test.c -o material_adicional/test.out
	
clean:
	rm -f *.out