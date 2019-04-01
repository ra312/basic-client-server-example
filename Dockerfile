FROM clang++:10.0.0
COPY . ~/docker_images
WORKDIR ~/docker_images
RUN clang++ -std=c++11 -stdlib=libc++ client_class.cpp -o client_class
CMD ["./client_class"]
