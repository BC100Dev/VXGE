#ifndef VXGE_PACKAGEFILE_HPP
#define VXGE_PACKAGEFILE_HPP

#include <filesystem>

namespace fs = std::filesystem;

namespace VX {

    class PackageEntry {
    protected:
        explicit PackageEntry();

        friend class PackageFile;
    };

    class VXPackage {
    public:
        explicit VXPackage(const fs::path& pkgFile);

        PackageEntry GetPackageEntry(const std::string& pkgPath);

    private:
        fs::path pkgFile;
    };

}

#endif //VXGE_PACKAGEFILE_HPP
