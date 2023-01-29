workspace(name = "com_google_protobuf_examples")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# This com_google_protobuf repository is required for proto_library rule.
# It provides the protocol compiler binary (i.e., protoc).
#
# We declare it as local_repository so we can test changes
# before they get merged. You'll want to use the following instead:
#
# http_archive(
#     name = "com_google_protobuf",
#     sha256 = "c29d8b4b79389463c546f98b15aa4391d4ed7ec459340c47bffe15db63eb9126",
#     strip_prefix = "protobuf-3.21.3",
#     urls = ["https://github.com/protocolbuffers/protobuf/archive/v3.21.3.tar.gz"],
# )

local_repository(
    name = "com_google_protobuf",
    path = "/Downloads/protobuf-3.21.9",
)

# Similar to com_google_protobuf but for Java lite. If you are building
# for Android, the lite version should be preferred because it has a much
# smaller code size.
#local_repository(
#    name = "com_google_protobuf_javalite",
#    path = "..",
#)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()