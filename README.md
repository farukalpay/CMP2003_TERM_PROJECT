# FindFast

CMP2003 dönem projesi. Belgelerde kelime aramak için zincirleme hash tablosu kullanır.

```sh
make run   # örnek aramalar
make app   # belgelerde arama
make test  # doğruluk testleri
```

C++17 ve Make gerekir. Test ve teslim hazırlığı için Python 3 kullanılır.

Kod [findfast.cpp](findfast.cpp) dosyasında. Öğrenci uygulaması `StudentIndex` sınıfıdır; hocanın verdiği diğer bölümler korunur. [Kısa açıklama ve sunum notları](REHBER.md).

`make lab` ders örneklerini, `make compare` tablo boyutu karşılaştırmasını açar. İş yükünü değiştirmek için örneğin `make bench N=10000 QUERIES=200000` kullanılabilir.

Teslim için `make submission` çalıştır. Yüklenecek dosya `build/submission/findfast.cpp` olur. Derlenen programlar ve teslim kopyaları Git'e eklenmez.
