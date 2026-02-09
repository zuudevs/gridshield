Role: Anda adalah Senior Technical Writer dan Web Engineer berpengalaman.

Tugas: Buatkan konten dokumentasi lengkap untuk project saya dengan struktur file spesifik di bawah ini. Gunakan format Markdown profesional.

Informasi Project:
- Nama Project: gridshield
- Multi-layer Security for AMI Systems
- Target Pengguna

Struktur File yang Wajib Dibuat (Satu per satu):

1. **root/README.md**
   - Buatkan Header dengan Nama Project dan Badges (License, Build Status, C++ Version).
   - Intro yang menarik ("Hook").
   - Key Features (List fitur utama).
   - Cuplikan kode singkat (Showcase).

2. **root/LICENSE**
   - Gunakan MIT License.

3. **root/SECURITY.md**
   - Jelaskan kebijakan keamanan standar.
   - Arahkan pelaporan bug keamanan ke email atau issue tracker private.

4. **root/CONTRIBUTING.md**
   - Panduan Pull Request.
   - Style guide 

5. **root/CODE_OF_CONDUCT.md**
   - Gunakan standar Contributor Covenant versi ringkas.

6. **root/BUILD.md**
   - Instruksi build langkah demi langkah.
   - Prasyarat
   - Contoh command line untuk build

7. **docs/ARCHITECTURE.md**
   - Jelaskan High-Level Design.
   - **PENTING:** Buatkan kode diagram `mermaid` (flowchart atau class diagram) yang menggambarkan bagaimana sistem ini bekerja (misal: alur data dari input ke output).

8. **docs/QUICKSTART.md**
   - Tutorial "Hello World" menggunakan library ini.
   - Sertakan kode lengkap yang bisa langsung dicompile user.

9. **docs/API.md**
   - Buatkan template struktur untuk referensi API (List Class utama dan Fungsi penting).

10. **docs/ROADMAP.md**
    - List rencana fitur masa depan (Todo list).

Tone: Professional, Concise, Technical, and Developer-friendly.
Bahasa: International

| daftar file																				 |
|--------------------------------------------------------------------------------------------|
| path						| deskripsi												| status |
|---------------------------|-------------------------------------------------------|--------|
| root/README.md 			| Halaman muka (Intro, Features, Badges) 				| ✅    |
| root/LICENSE              | MIT/GPL/Apache										| ✅    |
| root/SECURITY.md          | Cara lapor celah keamanan								| ✅    |
| root/CONTRIBUTING.md      | Cara berkontribusi									| ✅    |
| root/CODE_OF_CONDUCT.md   | Aturan perilaku										| ❎    |
| root/BUILD.md             | Cara build project									| ❎    |
| root/docs/ARCHITECTURE.md | High-level design (diagram, flow)						| ❎    |
| root/docs/CHANGELOG.md    | History perubahan versi								| ❎    |
| root/docs/QUICKSTART.md   | Tutorial singkat 5 menit								| ❎    |
| root/docs/API.md          | Referensi teknis mendalam								| ❎    |
| root/docs/ROADMAP.md      | Rencana masa depan									| ❎    |
