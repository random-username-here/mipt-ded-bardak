// #pragma once

// struct PersonCtlGfxTexturePackPathes {
//     std::string_view left;
//     std::string_view right;
//     std::string_view down;
//     std::string_view up;
// };

// class PersonCtlGfx {
//     AssetId left = 0;
//     AssetId right = 0;
//     AssetId down = 0;
//     AssetId up = 0;


// public:
//     PersonCtlGfx() = default; 

//     void load_assets(AssetManager *assets, PersonCtlGfxTexturePackPathes &texturePack) {
//         left = assets->addTexture(texturePack.left);
//         if (left == kInvalidAssetId) {
//             throw ModManager::Error("Failed to register unit left texture");
//         }

//         right = assets->addTexture(texturePack.right);
//         if (right == kInvalidAssetId) {
//             throw ModManager::Error("Failed to register unit right texture");
//         }

//         down = assets->addTexture(texturePack.down);
//         if (down == kInvalidAssetId) {
//             throw ModManager::Error("Failed to register unit down texture");
//         }

//         up = assets->addTexture(texturePack.up);
//         if (up == kInvalidAssetId) {
//             throw ModManager::Error("Failed to register unit up texture");
//         }
//     }

//     AssetId assetId(Person *person) {
//         switch (person->m_dir)
//         {
//             case Person::rotationDir::down: return down;
//             case Person::rotationDir::up: return up;
//             case Person::rotationDir::right: return right;
//             case Person::rotationDir::left: return left;
//             default: return down;
//         }
//     }
// };