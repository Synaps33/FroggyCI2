#pragma once

namespace oci {

struct SaveData {
    bool valid = false;
    int score = 0;
    int lives = 3;
    int missiles = 3;
    int gun_type = 0;
    int gun_level = 1;
    int level = 1;
};

namespace SaveGame {
    // Reads and validates the save file. Returns false if there is no valid save.
    bool Load(SaveData& out);
    bool Exists();
    // Overwrites the save file.
    void Write(const SaveData& data);
    // Writes a non-valid save (used on game over / new game).
    void Invalidate();

    // Autosave bookkeeping used by the levels manager.
    void MarkDirty();
    bool IsDirty();
    void ClearDirty();
}

} // namespace oci
