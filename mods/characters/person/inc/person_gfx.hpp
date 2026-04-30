#include "person.hpp"

struct PersonTexturePackPathes {
    std::string_view left;
    std::string_view right;
    std::string_view down;
    std::string_view up;
};

struct PersonAssetPack {
    AssetId left;
    AssetId right;
    AssetId up;
    AssetId down;

    void load_assets(AssetManager *assets, PersonTexturePackPathes &texturePack) {
        left = assets->addTexture(texturePack.left);
        if (left == kInvalidAssetId) {
            throw ModManager::Error("Failed to register unit left texture");
        }

        right = assets->addTexture(texturePack.right);
        if (right == kInvalidAssetId) {
            throw ModManager::Error("Failed to register unit right texture");
        }

        down = assets->addTexture(texturePack.down);
        if (down == kInvalidAssetId) {
            throw ModManager::Error("Failed to register unit down texture");
        }

        up = assets->addTexture(texturePack.up);
        if (up == kInvalidAssetId) {
            throw ModManager::Error("Failed to register unit up texture");
        }
    }
};

class PersonGfx : public Person {
    PersonAssetPack *assetPack_=nullptr;

public:
    PersonGfx(Map *map, Vec2i pos, size_t id, PersonAssetPack *assetPack): Person(map, pos, id), assetPack_(assetPack) {
        assert(assetPack_);
    } 
   
    AssetId assetId() const override {
        switch (dir())
        {
            case RotationDir::down: return assetPack_->down;
            case RotationDir::up: return assetPack_->up;
            case RotationDir::right: return assetPack_->right;
            case RotationDir::left: return assetPack_->left;
            default: return assetPack_->down;
        }
    }
};