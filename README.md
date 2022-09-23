# JR EAST TRAIN SIMULATOR PS1 電車でGO コントローラ 入力変換ソフトウェア (JC-PS101使用)

## 暫定リリースについて
* 自分の環境では動いたものは[こちら](https://fsmuseum.net/souvenir/trainsimutil/DGoController2KeyPress.zip)または[Release](https://github.com/RUTE-FSMUSEUM/jre_train_sim_dgo_controller_converter/releases/tag/v0.0.0)で公開しています
* PS1時代の**電車でGO！コントローラ（ワンハンドルタイプ）＋JC-PS101**の組み合わせで動作します（はずです）
* コントローラを動かすと、[Q]、[Z]、[S]、[1]などがキー入力されます（メモ帳などで入力結果は確認できます）
* DirectInputの再配布可能なsampleコードをベースにそれを改変して、実装しています（ソースコードは、整理してからアップします）
* 作者以外の環境では動かないという可能性もかなりありますので、その際はissueにてご報告ください、解決に向けて可能な限りトライしてみます（コントローラのマッチングをGUIDというもので行っているので、もしかしたら皆さまのPCでは動かないなどということがあり得るかもしれません）
* インテルのi7-7700のマシンでビルドしています
* 細かいバグ、UIなどは今後修正します（とりあえず自分も早くコントローラでプレイしたくて作っただけのものなので、細かい点はご容赦ください）

## 使い方
* コントローラ(JC-PS101)をPCに接続
* ソフトウェア(Joystic.exe)を起動
* JR EAST TRAIN SIMULATORを起動し、京浜東北線を選択（ワンハンドルタイプのみ対応のため）
* Loadingが終了したら、コントローラを**非常の位置に入れる**
* JR EAST TRAIN SIMULATORのゲーム画面で、非常ブレーキが有効になったことを確認して、あとは自由に操作してください

## 注意点
* 素早くコントローラを操作すると、正しく動作しなくなる可能性があります
* ゲームプレイ中に、正しく動作しなくなった（＝コントローラのハンドルの位置がゲーム画面と同期しなくなった）場合は、コントローラを**B1→切の順に操作する** or **非常の位置にする**ことで位置を再同期することができると思います
* ソフトウェア(Joystic.exe)を起動後、JC-PS101を認識できない場合は、"Joystick not found. The sample will now exit."というエラーダイアログが表示され、プログラムが終了します（JC-PS101をUSBハブに装着しているにもかかわらず、このエラーメッセージが出る場合は、issueにてご連絡ください）

## 動作確認環境
### PC
* Windows10 (エディション	Windows 10 Home, バージョン	21H2, OS ビルド	19044.2006)
* CPU Intel Core i7-7700
* RAM16GB
### コントローラ
* ELECOM JC-PS101UBK
* TAITO 電車でGO! コントローラ ワンハンドルタイプ PS
### ゲーム
* JR EAST TRAIN SIMULATOR (Build ID 9531495)

## Misc
* 本文に記載の企業名、商標、トレードマークと、本ソフトウェアの間には、一切の関係はありません（認可なども受けておりません）
* 本ソフトウェアは、AS-ISかつすべて自己責任でご使用ください（いかなるトラブルに対し、FSMUSEUMは保証いたしかねます）
