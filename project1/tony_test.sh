#s script tests Project 1
# Made by Tony Kim
# 20121022

LOCALS="0.0.0.0"
TSERVER="143.248.56.16"

if [ ! -d ./samples ]
then 
        echo "Make sure you have /samples directory and have your stuff there"
        exit 1
fi

function generate {
        tr A-Z a-z < ./samples/$1.txt > ./samples/$1_decrypted.txt
}

generate sample
generate crossbow
generate burrito


echo "Test script starting..."
echo "Don't forget to turn on your local server before running this script!"

function diffTest {
    if [ "$DIFF" != "" ] 
        then
        echo "Test failed!"
        exit 1
        fi
}
# Large file
echo "Now testing large size!"
echo "Creating large file..."
LC_CTYPE=C tr -dc A-Za-z0-9 </dev/urandom | head -c 50000000 > a.txt
echo "Large file created!"
tr A-Z a-z < a.txt > a_decrypted.txt

function large {
        ./client -h $1 -p 3000 -o 0 -s 3 < a.txt > a_encrypted_mine.txt
        ./client -h $1 -p 3000 -o 1 -s 3 < a_encrypted_mine.txt > a_decrypted_mine.txt
        CMP=$(cmp a_decrypted.txt a_decrypted_mine.txt)
        if [ "$CMP" != "" ] 
        then
        echo "Large Test failed!"
        exit 1
        fi
}

large $TSERVER
echo "Large file testserver pass"


function clilent {
        # 1
        ./client -h $1 -p 3000 -o 0 -s 3 < samples/sample.txt > sample_encrypted_mine.txt
        DIFF=$(diff samples/sample_encrypted.txt sample_encrypted_mine.txt)
        diffTest

        ./client -h $1 -p 3000 -o 1 -s 3 < samples/sample_encrypted.txt > sample_decrypted_mine.txt
        DIFF=$(diff samples/sample_decrypted.txt sample_decrypted_mine.txt)
        diffTest

        # 2
        ./client -h $1 -p 3000 -o 0 -s 3 < samples/crossbow.txt > crossbow_encrypted_mine.txt
        DIFF=$(diff samples/crossbow_encrypted.txt crossbow_encrypted_mine.txt)
        diffTest

        ./client -h $1 -p 3000 -o 1 -s 3 < samples/crossbow_encrypted.txt > crossbow_decrypted_mine.txt
        DIFF=$(diff samples/crossbow_decrypted.txt crossbow_decrypted_mine.txt)
        diffTest

        # 3
        ./client -h $1 -p 3000 -o 0 -s 3 < samples/burrito.txt > burrito_encrypted_mine.txt
        DIFF=$(diff samples/burrito_encrypted.txt burrito_encrypted_mine.txt)
        diffTest

        ./client -h $1 -p 3000 -o 1 -s 3 < samples/burrito_encrypted.txt > burrito_decrypted_mine.txt
        DIFF=$(diff samples/burrito_decrypted.txt burrito_decrypted_mine.txt)
        diffTest
}

function large {
        ./client -h $1 -p 3000 -o 0 -s 3 < a.txt > a_encrypted_mine.txt
        ./client -h $1 -p 3000 -o 1 -s 3 < a_encrypted_mine.txt > a_decrypted_mine.txt
        CMP=$(cmp a_decrypted.txt a_decrypted_mine.txt)
        if [ "$CMP" != "" ] 
        then
        echo "Test failed!"
        exit 1
        fi
}

# Client-TestServer
clilent $TSERVER
echo "Client-TestServer test passed!"

# Client-LocalServer
#clilent $LOCALS
#echo "client-LocalServer test passed!"

# Large file
echo "Now testing large size!"
echo "Creating large file..."
LC_CTYPE=C tr -dc A-Za-z0-9 </dev/urandom | head -c 50000000 > a.txt
echo "Large file created!"
tr A-Z a-z < a.txt > a_decrypted.txt


# Large size, Testserver
large $TSERVER
echo "Largefile-TestServer test passed!"

# Large size, Local Server
#large $LOCALS
#echo "Largefile-LocalServer test passed!"

echo "ALL tests passed! Hurray!"

# delete test files
rm sample_encrypted_mine.txt
rm sample_decrypted_mine.txt
rm samples/sample_decrypted.txt
rm samples/crossbow_decrypted.txt
rm samples/burrito_decrypted.txt
rm crossbow_encrypted_mine.txt
rm crossbow_decrypted_mine.txt
rm burrito_encrypted_mine.txt
rm burrito_decrypted_mine.txt
rm a_encrypted_mine.txt
rm a_decrypted_mine.txt
rm a_decrypted.txt
rm a.txt
echo "cleaned up!"
