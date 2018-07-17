{
    "targets": [
        {
            "target_name": "hyperscan",
            "sources": [
                "src/binding.cpp",
                "src/chimera_database.cpp"
            ],
            "include_dirs": [
                "<!(node -e \"require('nan')\")",
                "library_source/src",
                "library_source/chimera"
            ],
            "link_settings": {
                "libraries": [
                    "-lstdc++",
                    "../library_build/lib/libchimera.a",
                    "../library_build/lib/libpcre.a",
                    "../library_build/lib/libhs.a"
                ]
            },
            "xcode_settings": {
                "MACOSX_DEPLOYMENT_TARGET":"10.13"
            }
        }
    ]
}
