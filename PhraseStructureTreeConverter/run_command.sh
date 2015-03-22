#!/bin/bash

pushd `dirname $0` > /dev/null
SCRIPTPATH=`pwd`
popd > /dev/null

#echo ${SCRIPTPATH}
Stanford_Parser_JAR=${SCRIPTPATH}/lib/stanford-parser.jar

runtime_classpath=${SCRIPTPATH}/bin:${Stanford_Parser_JAR}

javac -classpath ${Stanford_Parser_JAR} -sourcepath src -d bin src/StanfordHeadRuleFinder.java 

java -Dfile.encoding=UTF-8 -classpath ${runtime_classpath} StanfordHeadRuleFinder $1 $2 $3 $4 $5 $6 $7
