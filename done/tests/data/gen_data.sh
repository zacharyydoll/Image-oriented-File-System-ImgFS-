cmd=$1
out=$2

files=(empty test02 test03 test04 test05 test06 test07 test08 test09 test10 test11 test12 test13)

for f in ${files[@]}
do
    ../../../done/imgfscmd ${cmd} "${f}.imgfs" > ${out}/${f}.txt
done
