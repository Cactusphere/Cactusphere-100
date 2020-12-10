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

### InitialFirmware
|フォルダー名|説明|
|:--|:--|
|DIInitialFirmware|接点入力モデル初期ファーム|
|RS485InitialFirmware|RS485モデル初期ファーム|

### DeviceTemplate
|フォルダー名|説明|
|:--|:--|
|DI|接点入力モデルIoT Centralデバイステンプレート|
|RS485|RS485モデルIoT Centralデバイステンプレート|


## ビルド時注意事項

### 高度なアプリケーション

#### 開発環境

##### Visual Studio Code

高度なアプリケーションビルドの際に開くフォルダーが異なります。

|モデル名|フォルダー名|
|:--|:--|
|接点入力モデル|HLApp/Cactusphere_100/atmarktechno_DI_model|
|RS485モデル|HLApp/Cactusphere_100/atmarktechno_RS485_model|

##### Visual Studio

高度なアプリケーションビルドの際には、下記のフォルダーを開いてください。
* HLApp/Cactusphere_100

また、ビルドを行うモデルの切り替えは下記の通り構成を切り替えてください。

|モデル名|構成|
|:--|:--|
|接点入力モデル|AtmarkTechno_DI_Debug or AtmarkTechno_DI_Release|
|RS485モデル|AtmarkTechno_RS485_Debug or AtmarkTechno_RS485_Release|

#### app_manifest.json

どちらの開発環境でも、修正する app_manifest.json は各モデル毎に下記の通りとなります。

|モデル名|ファイルパス|
|:--|:--|
|接点入力モデル|HLApp/Cactusphere_100/atmarktechno_DI_model/app_manifest.json|
|RS485モデル|HLApp/Cactusphere_100/atmarktechno_RS485_model/app_manifest.json|

## ツール

[Azure Sphere Explorer](https://github.com/KMOGAKI/Cactusphere-100/tree/explorer_rc-1/Tools/AzureSphereExplorer)
