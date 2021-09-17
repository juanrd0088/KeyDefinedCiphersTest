
fichero = "quijotecifmd54.txt"
salida = "conteo.txt"
file = open(fichero, 'r')
counts = dict()

for line in file.readlines():
	words = line.split()
	for word in words:
		if word in counts:
			counts[word] += 1
		else:
			counts[word] = 1

fout = open(salida,'w')
for i in counts:
	#print(i,counts[i])
	fout.write("'"+i +"'"+"	"+ str(counts[i])+ "\n")
	
