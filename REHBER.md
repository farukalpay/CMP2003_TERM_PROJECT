# Kod ve sunum notları

Program `kelime → liste kimliği` eşlemesi tutar. Belge motoru, bu kimlikle kelimenin geçtiği belgeleri bulur. Liste kimliği tek bir belge numarası değildir.

## Veri yapısı

Her hash gözü bir tek bağlı listenin başını gösterir. Bir düğümde kelime, liste kimliği ve sonraki düğümün adresi vardır.

```text
bir göz → A → B → C → nullptr
```

`StudentIndex(boyut)` göz sayısını kurucudan alır. Hocanın parametresiz fabrika çağrısı için varsayılan boyut 1024'tür. Seçilen boyut nesnenin ömrü boyunca aynı kalır.

Kelimenin karakterleri sırayla işlenir: `değer = değer * 31 + karakter`. Sonra `% tabloBoyutu` ile göz seçilir. `31` hash formülünün çarpanıdır. Hash eşitliği kelime eşitliği anlamına gelmediğinden zincirde tam kelimeler karşılaştırılır.

| İşlem | Yaptığı iş |
| --- | --- |
| `put` | Kelime varsa değerini günceller; yoksa başa düğüm ekler. |
| `find` | İlgili zinciri dolaşır. Bulamazsa çıktı parametresi değişmez. |
| `erase` | Önceki ve mevcut düğümü izleyip bağlantıyı düzeltir, düğümü siler. |
| `size` | Farklı kelime sayısını döndürür. |
| `clear` | Her zincirin baş düğümünü sırayla çıkarır ve sayacı sıfırlar. |

Kurucu baş dizisini ayırır. Yıkıcı önce düğümleri, sonra baş dizisini bırakır. Tahsislerde hocanın `memory` yardımcıları kullanılır. `Record` içindeki `Key`, kelimenin kendi kopyasını tutar.

Kelimenin hash hesabı `O(L)` karakter işi yapar. Sabit uzunluklu kelimeler ve iyi dağılım kabul edildiğinde ortalama düğüm işi `O(1+n/m)` olur: `n` kayıt, `m` göz sayısı. Bütün kayıtlar çakışırsa zincir araması `O(n)` olur. Temizleme `O(m+n)` sürer.

## Derste verilen örnekler

`examples/` içindeki sequential, binary ve interpolation aramaları indeks veya −1 döndürür. Linear probing `key % HTSize` ile başlar, dolu gözde sonraki gözü dener. Chaining yeni anahtarı listenin başına koyar. Bu dosyadaki sabit diziler örnek girdilerdir; arama fonksiyonları verilen girdi üzerinde çalışır.

Tamsayı laboratuvarında tekrar anahtar reddedilir. FindFast'in `put` sözleşmesi ise mevcut kelimenin değerini günceller.

## Sunumda göster

1. `make run` ile tek kelime, AND ve OR sonuçlarını aç.
2. `StudentIndex` içinde düğümü, başa eklemeyi ve silmedeki iki işaretçiyi göster.
3. A→B→C zincirinden B'yi silmeyi çiz: A'nın `next` alanı C'yi gösterir.
4. `make compare` ile aynı yöntemin farklı tablo boyutlarını göster. Süreler yedi çalıştırmanın ortalamasıdır.
5. `make test` sonucunu aç.

## Kontrol ve teslim

`make test` verilen 20 testi, ek testleri ve korunan kod kontrolünü çalıştırır. `make sanitize` bellek hatalarını denetler. Korunan bölümlerin dosya özetleri orijinal başlangıç dosyasından alınmıştır; `make_index`, motor, verilen testler ve `main` değişirse kontrol başarısız olur.

`make submission`, kaynağı tek başına normal ve sanitizer derlemesinde doğrulayıp teslim kopyasını `build/submission/` içine yazar. Kaynakta değişiklik yaptıktan sonra bu komutu yeniden çalıştır. Hazırlama komutu siteye yükleme yapmaz.

Kod ve metin hazırlanırken ChatGPT/Codex desteği kullanıldı.
