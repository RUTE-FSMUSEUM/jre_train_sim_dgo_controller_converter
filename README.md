# JR EAST Train Simulator | JR東日本トレインシミュレータ - コントローラ 入力変換ソフトウェア
最新版：**v0.5.1** </br></br>
JC-PS101 / ZUIKI 電車でＧＯ！！専用ワンハンドルコントローラ などのデバイスからの信号を変換し、特定のキー入力を行うソフトウェア、DgoConverter のリポジトリです</br>
<img width="256" src="https://user-images.githubusercontent.com/63990380/202198049-5252c8f0-ef44-4b92-89e3-567e429ac5d9.png"></img></br>
**▲ GUI例**</br>


## 主な機能
* ZUIKI 電車でＧＯ！！専用ワンハンドルコントローラー for Nintendo Switch に対応
* PlayStation 1用 電車でGO! コントローラ + ELECOM JC-PS101 の組み合わせに対応
* 電車でGO! コントローラ　ワンハンドルタイプ・ツーハンドルタイプの両方に対応
* キー入力は、JR東日本トレインシミュレータ （音楽館）および　Hmmsim Metro（Jeminie Interactive）に対応
* ソースコードを書き換えることで、キー入力発火条件（トリガマップ）の変更、入力キーマッピングの変更や JC-PS101 以外のゲームパッドコンバータ（別途ドライバをご用意ください）に対応可

## リリース
### ダウンロード
* [こちら](https://github.com/RUTE-FSMUSEUM/jre_train_sim_dgo_controller_converter/releases/tag/v0.5.1)で公開しています
### 最新版の変更点
* ZUIKI 電車でＧＯ！！専用ワンハンドルコントローラー for Nintendo Switch に対応しました
* [キーボード入力の発火条件（トリガマップ）](https://github.com/RUTE-FSMUSEUM/jre_train_sim_dgo_controller_converter/blob/af5893fbb713d0f9c94822b20818e0cd463da711/DgoConverter.cpp#L100)が編集可能になりました（編集の反映にはご自身で再ビルドが必要です）
* UI を一部変更しました

## 使い方の手順（JC-PS101＋電車でGO！コントローラ）
### JR東日本トレインシミュレータ
1. JC-PS101 と電車でGO！コントローラを接続
2. JC-PS101 を PC に接続
3. 本ソフトウェア（DgoConverter.exe）を起動
4. Device: から **JC-PS101U** を選択
5. コントローラを P5 に入れ、Dgoconverter 画面の X Axis（Dgo controller state の左上の項目）の値を確認する
6. **-24** となっている場合はEvent Trigger Map:から **JC-PS101U** を選択し、**-8** となっている場合は **JC-PS101U - late** を選択
7. JR東日本トレインシミュレータを起動
8. ワンハンドル（東海道線・中央線快速電車など）の場合は、Target Train Sim: を **JRE SIM ONE HANDLE** に、ツーハンドル（大糸線など）の場合は、Target: を **JRE SIM TWO HANDLE** に設定
9. ローディングが終了したら、コントローラを**非常の位置**に入れる
10. JR東日本トレインシミュレータ上で非常ブレーキが有効になったことを確認
11. DgoConverter.exe 上の Est. mascon state が EB になったことを確認
* C ボタンで電子警笛、B ボタンに空気笛を設定しています
### HMMSIM METRO
1. JC-PS101 と電車でGO！コントローラを接続
2. JC-PS101 を PC に接続
3. 本ソフトウェア（DgoConverter.exe）を起動
4. Device: から **JC-PS101U** を選択
5. コントローラを P5 に入れ、Dgoconverter 画面の X Axis（Dgo controller state の左上の項目）の値を確認する
6. **-24**となっている場合は Event Trigger Map: から **JC-PS101U** を選択し、**-8**となっている場合は **JC-PS101U - late** を選択
7. HMMSIM METRO を起動
8. Target Train Sim: から **HMMSIM METRO** に設定
9. コントローラを**通常ブレーキ8の位置**に入れる
10. ローディングが終了したら、HMMSIM METRO 上で非常ブレーキが有効になっていることを確認
11. コントローラを**通常ブレーキ8の位置**に入れ、HMMSIM METRO 上で**非常ブレーキが解除された**ことを確認（ベルが鳴るはずです）
12. DgoConverter.exe 上の Est. mascon state が B7 になったことを確認
* **HMMSIM METRO 上でキーマッピングをカスタマイズしている場合は、正しく動作しません**
* A ボタンに加速を緩める、B ボタンに空気笛、SELECT ボタンに DSD を設定しています
* **DSD コントロールはオフにする**ことをオススメします（試してみましたが、DSD オンではかなり操作がしにくいです）
* HMMSIM METRO に登場する車両は、非常ブレーキ1段、通常ブレーキ7段、力行（ノッチ）4段です
* コントローラの**非常ブレーキは使用しません**
* コントローラの**B8が非常ブレーキ**に対応しています
* コントローラの**加速5（ノッチ5）は使用しません**（入力してもなにも起こりません）
* HMMSIM METRO では、ノッチ2～4段はキーの長押しにより実現します（キーを連続時間押下する必要があります）
* ノッチ2～4段では、ZキーまたはAキーが0.07秒間押下されます
* ノッチ2～4段で、ノッチ操作を素早く操作した場合、とりわけ正しく動作しなくなる可能性が高いのでご注意ください
* ノッチがニュートラル（切）まで戻らなくなった場合は、コントローラを切の位置にした上で、HMMSIM METRO の電車がニュートラル状態になるまで、**A ボタンを長押ししてください**
* 以上を踏まえまして、HMMSIM METRO ではより一層丁寧でゆっくりとしたコントローラ操作を強く推奨いたします


## 使い方の手順（ZUIKI 電車でＧＯ！！専用ワンハンドルコントローラー ）
### JR東日本トレインシミュレータ
1. ZUIKI 電車でＧＯ！！専用ワンハンドルコントローラー 
3. 本ソフトウェア（DgoConverter.exe）を起動
4. Device: から **One Handle MasCon for Nintendo Switch** を選択
6. Event Trigger Map:から **ZUIKI One Handle Mascon** を選択
7. これより先は、[こちら](https://github.com/RUTE-FSMUSEUM/jre_train_sim_dgo_controller_converter#jr%E6%9D%B1%E6%97%A5%E6%9C%AC%E3%83%88%E3%83%AC%E3%82%A4%E3%83%B3%E3%82%B7%E3%83%9F%E3%83%A5%E3%83%AC%E3%83%BC%E3%82%BF)の 7. 以降をご覧ください
* ただし、JC-PS101＋電車でGO！コントローラとは異なり、A ボタンで電子警笛、B ボタンに空気笛を設定しています
### HMMSIM METRO
1. ZUIKI 電車でＧＯ！！専用ワンハンドルコントローラー 
3. 本ソフトウェア（DgoConverter.exe）を起動
4. Device: から **One Handle MasCon for Nintendo Switch** を選択
6. Event Trigger Map:から **ZUIKI One Handle Mascon** を選択
7. これより先は、[こちら](https://github.com/RUTE-FSMUSEUM/jre_train_sim_dgo_controller_converter/edit/main/README.md#hmmsim-metro)の 7. 以降をご覧ください
* ただし、JC-PS101＋電車でGO！コントローラとは異なり、Y ボタンに加速を緩める、B ボタンに空気笛、- ボタンに DSD を設定しています

## 注意点
* コントローラの状態は、1秒あたり100回読み取られます
* 読み取り速度を上回る速さでマスコンを操作すると、DgoConverter.exe 上でマスコン状態が正しく推定できず、正しく動作しなくなる可能性があります
* DgoConverter.exe 上の推定マスコン状態が、JR東日本トレインシミュレータ上のマスコン位置と同期しなくなった場合は、コントローラを**B1→切の順に操作する** or **非常の位置にする**ことで位置を再同期することができます
* JC-PS101 の製造時期によって、出力信号が若干異なる場合があることが確認されているため（Issue #1 参照）、そのようなトラブルに遭遇した場合は、Issues にてご報告ください（過去の事例は、JC-PS101によって、ノッチ５での X Axis の値が24や-8という異なる値をとるというものでした）

## Misc
* コントローラを動かすと、[Q]、[Z]、[S]、[1]などのキー入力され、これはメモ帳などで結果の確認ができます
* インテルのi7-7700のマシンでビルドしており、ARM系アーキテクチャでの動作確認はできておりません（ご自身で再ビルドが必要になる可能性があります）
* 32bit用のソフトウェアが必要な場合は、slnファイルを開き、ご自身で再ビルドをお願いいたします。
* ソースコードは、Microsoft 社の DirectX SDK に含まれる DirectInput の再配布可能なサンプルコードをベースに、それを大幅改変したものです
* 先述の DirectX SDK のライセンス制約もございますので、MIT ライセンス等で配布可能かは現在確認中です
* 非商用目的でのソースコード改変および再配布は可能ですが、商用目的（寄付含む）でのご利用は現状お断りします
* 作者は、機能向上やバグ修正を目的とした再開発や意見交換を、皆様と活発に行いたいというスタンスですので、どうぞよろしくお願いします
* ご質問などがありましたら、Issues にてご連絡ください

## 動作確認環境
### PC
* Windows10 (エディション	Windows 10 Home, バージョン	21H2, OS ビルド	19044.2130)
* CPU Intel Core i7-7700
* RAM16GB
### 開発環境
* Microsoft Visual Studio Community 2017 (Version 15.9.50)
* Microsoft .NET Framework (Version 4.8.04084)
### コントローラ
* ELECOM JC-PS101UBK
* TAITO 電車でGO! コントローラ ワンハンドルタイプ PS
* TAITO 電車でGO! コントローラ PS (ツーハンドルタイプ)
* ZUIKI 電車でＧＯ！！専用ワンハンドルコントローラー for Nintendo Switch
### ゲーム
* JR EAST TRAIN SIMULATOR (Build ID 9531495)

## 謝辞
* 理解しやすく使いやすいサンプルコードを配布してくださっている Microsoft 社の DirectX 開発チームへ
* v0.0.1 時点でバグ報告を詳細にしてくださり、DgoConverter の機能向上に大きく貢献してくださったユーザの方へ
* Steam かつ PC でトレインシミュレータシリーズを開発・発売してくださった音楽館の皆様へ
* 時代に先駆けた企画を提案してくださったJR東日本の皆様へ
* 素晴らしいグラフィックで長距離を楽しめる Hmmsim Metro を開発・発売してくださった Jeminie Interactive の皆様へ
* 雰囲気抜群のコントローラを製造販売されているZUIKIの皆様へ

## 免責事項
* 本文に記載の企業名、商標、トレードマークと、本リポジトリの間には、一切の関係はありません（各企業へ本リポジトリについて問い合わせをすることもご遠慮ください）
* 本リポジトリや DgoConverter.exe の使用には、DirectX および DirectX SDKのライセンスに合意する必要があります（詳しくは、[こちら](https://www.microsoft.com/ja-JP/download/details.aspx?id=6812)を参照）
* DirectX SDKのライセンスにより、Windows、Xbox、Windows Mobile platforms 以外で DgoConverter.exe を動作させること、およびそれを目的とした再開発は禁止されていますので、再開発の場合は特に注意してください
* 本リポジトリおよび DgoConverter.exe は、AS-IS かつすべて自己責任でご使用ください（いかなるトラブルに対し、RUTE-FSMUSEUM は保証いたしかねます）
