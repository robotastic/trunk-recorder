protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` streamer.proto
protoc -I . --cpp_out=. streamer.proto
