# Cactusphere 100

このリポジトリには、Cactusphere 100シリーズ　ファームウェアのソースコードが含まれています。

## ビルド済バイナリ、Azure IoT Centralテンプレート

以下からダウンロードすることができます。

https://github.com/Cactusphere/Cactusphere-100/releases

## ソフトウェアマニュアル

ビルド方法等、詳細についてはソフトウェアマニュアルを参照してください。

ソフトウェアマニュアルのダウンロードには、Armadilloサイトのユーザー登録とログインが必要になります。

https://armadillo.atmark-techno.com/resources/documents/cactusphere/manuals

## フォルダー構成

### Firmware
|フォルダー名|説明|
|:--|:--|
|HLApp/Cactusphere_100|高度なアプリケーション(接点入力モデル、RS485モデル共通)|
|RTApp/DI|リアルタイム対応アプリケーション(接点入力モデル)|
|RTApp/RS485|リアルタイム対応アプリケーション(RS485モデル)|

* RS485モデルは動作未確認です。

### InitialFirmware

|フォルダー名|説明|
|:--|:--|
|DIInitialFirmware|接点入力モデル初期ファーム|

## ビルド時注意事項

開発環境 Visual Studio と Visual Studio Code では、高度なアプリケーションビルドの際に
開くフォルダーが異なります。

|開発環境|フォルダー名|
|:--|:--|
|Visual Studio|HLApp/Cactusphere_100|
|Visual Studio Code|HLApp/Cactusphere_100/atmarktechno_DI_model|

※ どちらの開発環境でも、修正する app_manifest.json は HLApp/Cactusphere_100/atmarktechno_DI_model を使用してください。
