for i in ${1-*.?}
do
	echo ====$i====
	egrep '^\.[a-z][a-z]' $i | egrep -v '^\.(ap|br|ds|el|fi|ft|ie|if|in|ne|nf|nh|nr|ps|pt|sp|ta|ti|vs)'
done
