
#!/usr/bin/env bash
set -e

if [ $# -eq 0 ]
  then
    echo "File name missing"
	exit
fi

file=$1

echo $1

output=`sudo ipfs add -r $1`
echo "output: $output"
hash_value=$(echo $output | cut -d ' ' -f 2)
echo "hash_value: $hash_value"
pin_output=`sudo ipfs pin add -r $hash_value`
echo $pin_output
