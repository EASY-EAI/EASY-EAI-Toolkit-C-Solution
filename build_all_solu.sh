#/bin/bash

set -e

SHELL_FOLDER=$(cd "$(dirname "$0")";pwd)
cd $SHELL_FOLDER

release_path="Release"
solu_list=`ls -d solu-*`

allsolu="$solu_list"

usage()
{
	echo "DESCRIPTION"
	echo "LMO toolkit-c-solu compile tool."
	echo " "
	echo "SYNOPSIS"
	echo "	./build_all_solu.sh [clear]"
}

clear_api()
{
	for var in ${allsolu[@]}
	do
		if [ -e $var/build.sh ]; then
			cd $var
			./build.sh clear
			cd - > /dev/null
		fi
	done
}

main() {
#	for var in ${allsolu[@]}
#	do
#		echo $var
#	done

	if [ $# -eq 1 ]; then
		if [ $1 == "clear" ]; then
			clear_api
		else
			usage
		fi

		exit 0
	fi
	
	### 编译所有demo
	for var in ${allsolu[@]}
	do
		if [ -e $var/build.sh ]; then
			cd $var
			./build.sh clear
			./build.sh
			cd - > /dev/null
		fi
	done
}

main "$@"

