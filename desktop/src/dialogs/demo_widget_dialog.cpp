// DemoWidgetDialog — a gallery of Qt widgets for theme preview.

#include "demo_widget_dialog.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

DemoWidgetDialog::DemoWidgetDialog(QWidget* parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle("Widget Gallery — Theme Preview");
    resize(650, 500);

    auto* layout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget;
    layout->addWidget(tabs);

    // ── Tab 1: Buttons & Inputs ──────────────────────────────
    {
        auto* page = new QWidget;
        auto* v = new QVBoxLayout(page);

        // Buttons
        auto* btnGroup = new QGroupBox("Buttons");
        auto* btnLayout = new QHBoxLayout(btnGroup);

        auto* primary = new QPushButton("Primary");
        btnLayout->addWidget(primary);

        auto* secondary = new QPushButton("Secondary");
        secondary->setProperty("class", QStringList{"secondary"});
        btnLayout->addWidget(secondary);

        auto* ghost = new QPushButton("Ghost");
        ghost->setProperty("class", QStringList{"ghost"});
        btnLayout->addWidget(ghost);

        auto* destructive = new QPushButton("Destructive");
        destructive->setProperty("class", QStringList{"destructive"});
        btnLayout->addWidget(destructive);

        auto* disabled = new QPushButton("Disabled");
        disabled->setEnabled(false);
        btnLayout->addWidget(disabled);

        v->addWidget(btnGroup);

        // Tool buttons
        auto* toolGroup = new QGroupBox("Tool Buttons");
        auto* toolLayout = new QHBoxLayout(toolGroup);
        for (const auto& label : {"Cut", "Copy", "Paste", "Undo", "Redo"}) {
            auto* tb = new QToolButton;
            tb->setText(label);
            toolLayout->addWidget(tb);
        }
        v->addWidget(toolGroup);

        // Input fields
        auto* inputGroup = new QGroupBox("Input Fields");
        auto* inputLayout = new QVBoxLayout(inputGroup);

        auto* lineEdit = new QLineEdit;
        lineEdit->setPlaceholderText("Type something here...");
        inputLayout->addWidget(lineEdit);

        auto* spinBox = new QSpinBox;
        spinBox->setRange(0, 100);
        spinBox->setValue(42);
        inputLayout->addWidget(spinBox);

        auto* comboBox = new QComboBox;
        comboBox->addItems({"Option A", "Option B", "Option C", "Option D"});
        inputLayout->addWidget(comboBox);

        v->addWidget(inputGroup);
        v->addStretch();
        tabs->addTab(page, "Buttons && Inputs");
    }

    // ── Tab 2: Checks, Radios, Sliders ──────────────────────
    {
        auto* page = new QWidget;
        auto* v = new QVBoxLayout(page);

        auto* checkGroup = new QGroupBox("Checkboxes");
        auto* checkLayout = new QVBoxLayout(checkGroup);
        auto* cb1 = new QCheckBox("Enable notifications");
        cb1->setChecked(true);
        checkLayout->addWidget(cb1);
        checkLayout->addWidget(new QCheckBox("Auto-save on exit"));
        checkLayout->addWidget(new QCheckBox("Show line numbers"));
        auto* cbDisabled = new QCheckBox("Disabled option");
        cbDisabled->setEnabled(false);
        checkLayout->addWidget(cbDisabled);
        v->addWidget(checkGroup);

        auto* radioGroup = new QGroupBox("Radio Buttons");
        auto* radioLayout = new QVBoxLayout(radioGroup);
        auto* r1 = new QRadioButton("Light mode");
        auto* r2 = new QRadioButton("Dark mode");
        r2->setChecked(true);
        auto* r3 = new QRadioButton("System default");
        radioLayout->addWidget(r1);
        radioLayout->addWidget(r2);
        radioLayout->addWidget(r3);
        v->addWidget(radioGroup);

        auto* sliderGroup = new QGroupBox("Sliders & Progress");
        auto* sliderLayout = new QVBoxLayout(sliderGroup);

        auto* slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100);
        slider->setValue(65);
        sliderLayout->addWidget(slider);

        auto* progress1 = new QProgressBar;
        progress1->setValue(75);
        sliderLayout->addWidget(progress1);

        auto* progress2 = new QProgressBar;
        progress2->setValue(30);
        sliderLayout->addWidget(progress2);

        v->addWidget(sliderGroup);
        v->addStretch();
        tabs->addTab(page, "Controls");
    }

    // ── Tab 3: Text & Lists ──────────────────────────────────
    {
        auto* page = new QWidget;
        auto* v = new QVBoxLayout(page);

        auto* textGroup = new QGroupBox("Text Editing");
        auto* textLayout = new QVBoxLayout(textGroup);

        auto* textEdit = new QPlainTextEdit;
        textEdit->setPlainText(
            "fn main() {\n"
            "    let theme = \"Synthwave '84\";\n"
            "    println!(\"Current theme: {}\", theme);\n"
            "    for i in 0..10 {\n"
            "        apply_style(i);\n"
            "    }\n"
            "}"
        );
        textLayout->addWidget(textEdit);
        v->addWidget(textGroup);

        auto* listGroup = new QGroupBox("List Widget");
        auto* listLayout = new QVBoxLayout(listGroup);
        auto* list = new QListWidget;
        list->addItems({
            "Synthwave '84", "Dracula", "One Dark Pro",
            "Monokai", "Solarized Dark", "Nord",
            "Gruvbox", "Tokyo Night", "Catppuccin"
        });
        list->setCurrentRow(0);
        listLayout->addWidget(list);
        v->addWidget(listGroup);

        tabs->addTab(page, "Text && Lists");
    }

    // ── Tab 4: Tree & Labels ─────────────────────────────────
    {
        auto* page = new QWidget;
        auto* v = new QVBoxLayout(page);

        auto* treeGroup = new QGroupBox("Tree Widget");
        auto* treeLayout = new QVBoxLayout(treeGroup);
        auto* tree = new QTreeWidget;
        tree->setHeaderLabels({"Name", "Type", "Size"});
        tree->setColumnCount(3);

        auto* root1 = new QTreeWidgetItem(tree, {"desktop", "folder", ""});
        new QTreeWidgetItem(root1, {"main.cpp", "file", "2.4 KB"});
        new QTreeWidgetItem(root1, {"application.cpp", "file", "8.1 KB"});
        auto* widgets = new QTreeWidgetItem(root1, {"widgets", "folder", ""});
        new QTreeWidgetItem(widgets, {"web_shell_widget.cpp", "file", "5.3 KB"});
        new QTreeWidgetItem(widgets, {"style_manager.cpp", "file", "3.2 KB"});

        auto* root2 = new QTreeWidgetItem(tree, {"web", "folder", ""});
        new QTreeWidgetItem(root2, {"package.json", "file", "1.1 KB"});
        auto* shared = new QTreeWidgetItem(root2, {"shared", "folder", ""});
        new QTreeWidgetItem(shared, {"themes.ts", "file", "2.8 KB"});

        tree->expandAll();
        treeLayout->addWidget(tree);
        v->addWidget(treeGroup);

        auto* labelGroup = new QGroupBox("Labels & Status");
        auto* labelLayout = new QVBoxLayout(labelGroup);
        labelLayout->addWidget(new QLabel("Regular label text"));
        auto* boldLabel = new QLabel("<b>Bold label</b>");
        labelLayout->addWidget(boldLabel);
        auto* mutedLabel = new QLabel("This would be muted text in a real app");
        mutedLabel->setEnabled(false);
        labelLayout->addWidget(mutedLabel);
        v->addWidget(labelGroup);

        v->addStretch();
        tabs->addTab(page, "Tree && Labels");
    }

    // ── Bottom: Close button ─────────────────────────────────
    auto* bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    auto* closeBtn = new QPushButton("Close");
    closeBtn->setProperty("class", QStringList{"secondary"});
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    bottomLayout->addWidget(closeBtn);
    layout->addLayout(bottomLayout);
}
