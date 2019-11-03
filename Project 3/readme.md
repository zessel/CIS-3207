Zach Essel

makefile commands:
    make Project3_Server
    make client

Project3_Server:

A spell check server that can run over a network

Prompts the user for a word and echos the word back with either
OK - word was found in the servers dictionary
MISSPELLED - word was not found in the servers dictionary

After checking the spelled the server ends the connection and logs the request to a
file "log.txt" in it's directory.  Log will be rewritten upon every launch of the server.

The server can be configured to use a custom dictionary or port or both as command line arguments
i.e.
./Project3_Server
./Project3_Server 12345
./Project3_Server /dictionaries/custom.txt
./Project3_Server 12345 /dictionaries/custom.txt

If either are not provided it runs a default port of 3207 and dictionary "words.txt"

Please note that custom dictionaries will only work if each word is ended with a LF (\n)

Loading a dictionary with CFLF (\r\n) will terminate the server with an helpful error

client:

The testing client uses it's own dictionary (clientdict.txt) of words and typos.  It spawns some number of workers
each worker querying a random entry from the dictionary.  The client assumes localhost and the server default port of 3207.