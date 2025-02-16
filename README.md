# ImgFS: Image-oriented File System
## Project description
ImgFS is a project designed to develop a large program in C that focuses on system-level programming. The project involved creating a command-line utility to manage images, inspired by Facebook's "Haystack" system.

This project was developed as part of **CS202: Computer Systems** at **École Polytechnique Fédérale de Lausanne (EPFL)**.

##### project web page: https://projprogsys-epfl.github.io/project/mainprj/01-main/
## Required Libraries

- `libssl-dev`: Cryptographic library used to compute image "hash"-codes.
- `libvips-dev`: Library to process images from C code.
- `libjson-c-dev`: Library to process JSON content from C code.
## Using ImgFS
- From the command line:
```
# For help
./imgfscmd help

# Create a new ImgFS file
./imgfscmd create <ImgFS Filename> 

# Insert an image into the file (data samples available in done/tests/data)
./imgfscmd insert <ImgFS Filename>  <Image ID>  <path/to/image>

# Read an image from the imgFS and save it to a file
./imgfscmd read <ImgFS filename>  <Image ID> [original | orig | thumbnail | thumb | small]

# Delete an image imageID from the ImgFS file
./imgfscmd delete <ImgFS filename>  <Image ID>

# Launching the server 
./imgfs_server <ImgFS filename>
```
## Submission information for graders
### Helper functions created
In `http_prot.c`:
- `http_parse_headers`
- `get_next_token`

Both of which help `http_parse_message`parse an HTTP message (see code documentation for more information). 

In Multiple files:
- `safe_free`

Helper function to better handle cleanup of pointers (see code documentation for required arguments and information).

In `tcp-test-client.c`:
- `send_file`

Helper function for cleaner code that sends a file to a server 

In `tcp-test-server.c`:
- `receive_file`

Helper function for cleaner code that receives a file over a TCP connection




