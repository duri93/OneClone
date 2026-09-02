#include "RCloneProvider.h"

QStringList RCloneProvider::buildCommand(const RcloneCommandParams& params) const
{
    QStringList command;

    if (params.type == "mount") {
        command << "mount" << params.remote << params.local;
        if (params.readOnly) command << "--read-only";
        command << "--vfs-cache-mode"              << params.cacheMode;
        command << "--vfs-cache-max-size"          << QString::number(params.cacheMaxSize)       + "G";
        command << "--vfs-cache-min-free-space"    << QString::number(params.cacheMinFreeSpace)  + "G";
        command << "--vfs-cache-max-age"           << QString::number(params.cacheMaxAge)        + "h";
        command << "--vfs-read-chunk-size"         << QString::number(params.readChunkSize)      + "M";
        command << "--vfs-read-chunk-size-limit"   << QString::number(params.readChunkSizeLimit) + "M";
    } else {
        command << params.type;
        if (params.swapSides) {
            command << params.remote << params.local;
        } else {
            command << params.local << params.remote;
        }
        command << "--progress" << "--delete-before";
    }

    command << "--buffer-size"                 << QString::number(params.bufferSize)         + "M";
    command << "--transfers"                   << QString::number(params.transfers);
    command << "--checkers"                    << QString::number(params.checkers);
    if (params.links) command << "--links";

    return command;
}
