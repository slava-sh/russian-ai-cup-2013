pushd `dirname $0` > /dev/null
SCRIPTDIR=`pwd`
popd > /dev/null

java -cp ".:*:$SCRIPTDIR/*" -jar "$SCRIPTDIR/local-runner.jar" "$SCRIPTDIR/local-runner-sync.properties" &
