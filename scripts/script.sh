#!/bin/bash


#controllo se tra gli argomenti e' stato passato -help
for ARG; do 
	if [ $ARG = "-help" ]; then
		echo "Usage:" 1>&2
		exit 1
	fi
done

#controllo che gli argomenti siano esattamente 2
if ! [ $# = 2 ]; then
	echo "Usage:" 1>&2
	exit 1
fi

CONFFILE=$1 #percorso del file di configurazione

#controllo che CONFFILE sia un file non speciale
if ! [ -f $CONFFILE ]; then
	echo "Usage:" 1>&2
	exit 1
fi

#estraggo il percorso della directory specificato in DirName
LINE=$(grep -v '#' $CONFFILE | grep -e DirName)
DIR=${LINE#*=}
DIR=${DIR%' '}

#controllo che DIR sia una directory
if ! [ -d $DIR ]; then
	echo "Usage:" 1>&2
	exit 1
fi

TIME=$2

#se TIME < 0 esce
if [ $TIME -lt 0 ]; then
	echo "Usage:" 1>&2
	exit 1
fi


FILES=$DIR/*
echo "DIR e' $DIR"
#se TIME == 0 stampa su stdout il contenuto di DIR
if [ $TIME = 0 ]; then 
	for FILE in $FILES; do
		echo $FILE
	done

else #se TIME > 0
	find $DIR -mmin +$TIME ! -path $DIR -exec tar -cvzf archive.tar.gz --remove-files {} + #| xargs rm  -vfd
fi

echo "Success!"
exit 0
# legge nel file il percorso corrispondente a dirname, archivia tutti i file 
# e directories che sono piu' vecchi di t minuti in un file con estensione 
#.tar.gz. In tal caso le cartelle e i file archiviati dovranno essere eliminati 
# t == 0 dovranno essere stampati su standard output tutti i file contenuti in
# DirName. 
