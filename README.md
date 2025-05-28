# comment-remove

[![Build Status](https://github.com/alxluk/comment-remove/actions/workflows/makefile.yml/badge.svg)](https://github.com/alxluk/comment-remove/actions)

Утилита для удаления комментариев из исходных файлов на разных языках программирования

## Описание
`comment-remove` — это инструмент, который удаляет все виды комментариев (однострочные и многострочные) из файлов с поддерживаемыми расширениями. Инструмент автоматически определяет язык программирования по расширению файла и удаляет соответствующие комментарии.

## Поддерживаемые языки
| Язык           | Расширения                               |
|----------------|------------------------------------------|
| C/C++/Java/JS  | `.c`, `.h`, `.cpp`, `.hpp` `.java`, `.js`|
| Python         | `.py`                                    |
| Shell/Perl/Ruby| `.sh`, `.pl`, `.rb`, `.bash`             |
| HTML/XML       | `.html`, `.htm`, `.xml`                  |
| CSS            | `.css`                                   |
| PHP            | `.php`                                   |

## Установка

### 1. Зависимости
Убедитесь что установлены: `diffutils` (для сравнения файлов)
```bash
sudo apt install diffutils  # Debian/Ubuntu
```
```bash
sudo yum install diffutils  # RHEL/CentOS
```
```bash
sudo pacman -S diffutils  # Arch/Manjaro
```
### 2. **Сборка с помощью Makefile**
```bash
make all
```
### 3. Использование
```bash
./comment-remove <путь_к_файлу>
```
Пример:
```bash
## Примеры использования
./comment-remove ./examples/example.py
./comment-remove ./examples/example.c
./comment-remove -y ./examples/example.xml
```
Что происходит после запуска?
1. Создаётся копия файла с суффиксом _1 (например: example_1.c)
2. Показывается разница между исходным и очищенным файлом (diff)
3. Предлагается заменить исходный файл на очищенный
