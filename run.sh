gcc -g -o main src/*.c -Iinclude
for i in 1 2 3 4 5 6; do
    ./main sample$i.txt > result/sample_result$i.txt
    ./main test$i.txt > result/test_result$i.txt
done
