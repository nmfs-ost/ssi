#include "dialogparameterview.h"
#include "ui_dialogparameterview.h"

//#include <cmath>
#include "ss_math.h"

DialogParameterView::DialogParameterView(QWidget *parent, bool showTrans) :
    QDialog(parent),
    ui(new Ui::DialogParameterView)
{
    ui->setupUi(this);
    setShowTrans(showTrans);
    setTitle("");
    parameters = nullptr;
    numParamsShown = 0;
    pLabel.append(nullptr); pLabel.clear();
    pMin.append(nullptr);   pMin.clear();
    pMax.append(nullptr);   pMax.clear();
    pInit.append(nullptr);  pInit.clear();
    pSlider.append(nullptr); pSlider.clear();
    sValue.append(nullptr);  sValue.clear();
    pType.append(nullptr);  pType.clear();
    eInput.append(nullptr); eInput.clear();
    clearAll();
    move(parent->pos());
    resize(805, height());

    ui->pushButton_apply->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_reset->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_close->setFocusPolicy(Qt::NoFocus);
    connect (ui->pushButton_apply, SIGNAL(clicked()), SLOT(apply()));
    connect (ui->pushButton_reset, SIGNAL(clicked()), SLOT(reset()));
    connect (ui->pushButton_close, SIGNAL(clicked()), SLOT(close()));
    connectAll();
    hide();
}

DialogParameterView::~DialogParameterView() {
    ui->labelParamMin->releaseKeyboard();
    clearAll();
    delete ui;
}

void DialogParameterView::clearAll() {
    if (pLabel.isEmpty())
        return;

    while (pLabel.count() > 0) {
        delete pLabel.takeLast();
        delete pMin.takeLast();
        delete pMax.takeLast();
        delete pInit.takeLast();
        delete pSlider.takeLast();
        delete sValue.takeLast();
        delete pType.takeLast();
        delete eInput.takeLast();
    }
}

void DialogParameterView::disconnectAll() {
    for (int i = 0; i < pMin.count(); i++) {
        disconnect (pMin.at(i), SIGNAL(valueChanged(double)), this, SLOT(minChanged(double)));
        disconnect (pMax.at(i), SIGNAL(valueChanged(double)), this, SLOT(maxChanged(double)));
        disconnect (pSlider.at(i), SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)));
        disconnect (sValue.at(i), SIGNAL(valueChanged(double)), this, SLOT(sValueChanged(double)));
    }
}

void DialogParameterView::connectAll() {
    for (int i = 0; i < pMin.count(); i++) {
        connect (pMin.at(i), SIGNAL(valueChanged(double)), this, SLOT(minChanged(double)));
        connect (pMax.at(i), SIGNAL(valueChanged(double)), this, SLOT(maxChanged(double)));
        connect (pSlider.at(i), SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)));
        connect (sValue.at(i), SIGNAL(valueChanged(double)), this, SLOT(sValueChanged(double)));
    }
}

void DialogParameterView::setTitle(QString title) {
    QDialog::setWindowTitle(QString("%1 Parameters").arg(title));
}

void DialogParameterView::setParameterTable(tablemodel *params)
{
    if (params != nullptr && params->rowCount() > 0) {
        parameters = params;
//        paramsChanged();
    }
}

void DialogParameterView::setupView(int num)
{
    int rownum = 0;
    int baseht = 174; int rowht = 35;
    position = pos();
    window = size();
    clearAll();
    disconnectAll();
    for (int i = 0; i < numParamsShown; i++) {
        rownum = i + 1;
        pLabel.append(new QLabel(parameters->getRowHeader(i), this));
        pMin.append(valueSpinBox());
        pMax.append(valueSpinBox());
        pInit.append(parameterSpinBox(false, false));
        pSlider.append(parameterSlider());
        sValue.append(parameterSpinBox());
        pType.append(new QLabel("Value", this));
        eInput.append(parameterSpinBox(false, false));

        ui->gridLayout->addWidget(pLabel.at(i), rownum, 0);
        ui->gridLayout->addWidget(pMin.at(i), rownum, 1);
        ui->gridLayout->addWidget(pMax.at(i), rownum, 2);
        ui->gridLayout->addWidget(pInit.at(i), rownum, 3);
        ui->gridLayout->addWidget(pSlider.at(i), rownum, 4);
        ui->gridLayout->addWidget(sValue.at(i), rownum, 5);
        if (transVisible) {
            ui->gridLayout->addWidget(pType.at(i), rownum, 6);
            ui->gridLayout->addWidget(eInput.at(i), rownum, 7);
        }
    }
    int ht = window.height();
    int newht = baseht + (num > 1? num-1: 0) * rowht;
    int ypos = position.ry() + ((ht - newht)/2);
    int xpos = position.rx();
    QPoint parpos = static_cast<QWidget *>(parent())->pos();
    window.setHeight(newht);
    position.setX((xpos == 0)? parpos.rx(): xpos);
    position.setY(ypos);
    resize(window);
    move(position);
    connectAll();
}

void DialogParameterView::setSliders()
{
    QStringList paramvalues;
    double min, max, init;

    for (int i = 0; i < numParamsShown; i++) {
        paramvalues = parameters->getRowData(i);
        min = paramvalues.at(0).toDouble();
        max = paramvalues.at(1).toDouble();
        init = paramvalues.at(2).toDouble();
        pMin.at(i)->setRange(min);
        pMin.at(i)->setValue(min);
        pMax.at(i)->setRange(max);
        pMax.at(i)->setValue(max);
        pInit.at(i)->setRange(min, max);
        pInit.at(i)->setValue(init);
        setSliderRange (i, min, max);
        setSlider(i, init);
        sValue.at(i)->setRange(min, max);
        sValue.at(i)->setValue(init);
        setInput(i);
    }
}

bool DialogParameterView::setInput(int pnum) {
    bool changed = false;
    if (pLabel.count() > pnum) {
        double value = sValue.at(pnum)->value();
        double oldValue = eInput.at(pnum)->value(), newValue = 0;
        QString txt = pType.at(pnum)->text();

        if (txt.contains("exp", Qt::CaseInsensitive)) {
            newValue = exp(value);
        }
        else if (txt.contains("log", Qt::CaseInsensitive)) {
            newValue = log(value);
        }
        else if (txt.contains("logist", Qt::CaseInsensitive)) {
            newValue = logist(value);
        }
        else { //if (txt.contains("value", Qt::CaseInsensitive)) {
            newValue = value;
        }
        if (newValue < oldValue || newValue > oldValue) {
            changed = true;
            setInputValue(pnum, newValue);
        }
    }
    return changed;
}

void DialogParameterView::setInputValue(int pnum, double value) {
    eInput.at(pnum)->setRange((value-5.0), (value+5.0));
    eInput.at(pnum)->setValue(value);
}

void DialogParameterView::setNumParamsShown(int num) {
    numParamsShown = num;
    setupView(num);
    paramsChanged();
}

int DialogParameterView::getNumParameters() {
    return numParamsShown;
}

void DialogParameterView::paramsChanged() {
    if (parameters != nullptr)
    {
        disconnectAll();
        setSliders();
        connectAll();
        sliderChanged(1);
    }
}

void DialogParameterView::setSliderRange(int pnum, double min, double max) {
    int intmin = static_cast<int>(min * 1000.0);
    int intmax = static_cast<int>(max * 1000.0);
    pSlider.at(pnum)->setRange(intmin, intmax);
}

void DialogParameterView::setSlider(int pnum, double value) {
    int intval = static_cast<int>(value * 1000.0);
    pSlider.at(pnum)->setValue(intval);
}

void DialogParameterView::setName(int pnum, QString name) {
    if (pLabel.count() > pnum) {
        pLabel.at(pnum)->setText(name);
        parameters->setRowHeader(pnum, name);
    }
}

void DialogParameterView::setType(int pnum, QString type) {
    if (pType.count() > pnum) {
        pType.at(pnum)->setText(type);
        setInput(pnum);
    }
}

void DialogParameterView::minChanged(double value) {
    double dMin, dMax;

    for (int i = 0; i < pLabel.count(); i++) {
        dMin = pMin.at(i)->value();
        dMax = pMax.at(i)->value();
        if (checkMinMax(&dMin, &dMax)) {
            pMin.at(i)->setValue(dMin);
            pMax.at(i)->setValue(dMax);
        }
        sValue.at(i)->setRange(dMin, dMax);
        setSliderRange(i, dMin, dMax);
    }
}

void DialogParameterView::maxChanged(double value) {
    double dMin, dMax;

    for (int i = 0; i < pLabel.count(); i++) {
        dMin = pMin.at(i)->value();
        dMax = pMax.at(i)->value();
        if (checkMinMax(&dMin, &dMax)) {
            pMin.at(i)->setValue(dMin);
            pMax.at(i)->setValue(dMax);
        }
        sValue.at(i)->setRange(dMin, dMax);
        setSliderRange(i, dMin, dMax);
    }
}

void DialogParameterView::sliderChanged(int value) {
    Q_UNUSED(value);
    bool changed = false;
//    int num = pSlider.count();
    for (int i = 0; i < pSlider.count(); i++) {
        int val = pSlider.at(i)->value();
        int iMin = static_cast<int>(pMin.at(i)->value() * 1000);
        int iMax = static_cast<int>(pMax.at(i)->value() * 1000);
        checkMinMax(&iMin, &iMax);
        int check = static_cast<int>(sValue.at(i)->value() * 1000.0);
        if (val > iMax) {
//            pSlider.at(i)->setValue(iMax);
            pSlider.at(i)->setRange(iMin, iMax);
        }
        else if (val < iMin) {
//            pSlider.at(i)->setValue(iMin);
            pSlider.at(i)->setRange(iMin, iMax);
        }
        else if (val != check)
        {
            double sval = static_cast<double>(val) / 1000.0;
            sValue.at(i)->setValue(sval);
            changed = true;
        }
    }
}


void DialogParameterView::sValueChanged(double value) {
    Q_UNUSED(value);
    bool changed = false;
    int num = pLabel.count();
    for (int i = 0; i < num; i++) {
        double sval = sValue.at(i)->value();
        double dMin = pMin.at(i)->value();
        double dMax = pMax.at(i)->value();
        if (checkMinMax (&dMin, &dMax)) {
            pMin.at(i)->setValue(dMin);
            pMax.at(i)->setValue(dMax);
        }
        int check = static_cast<int>(sval * 1000.0);
        int sliderval = pSlider.at(i)->value();
        if (sval > dMax) {
            sValue.at(i)->setRange(dMin, dMax);
//            sValue.at(i)->setValue(dMax);
        }
        else if (sval < dMin) {
            sValue.at(i)->setRange(dMin, dMax);
//            sValue.at(i)->setValue(dMin);
        }
        else {
            check = static_cast<int>(sValue.at(i)->value() * 1000);
            if (sliderval != check){
                pSlider.at(i)->setValue(check);
            }
            if (convertToInput(i))
                changed = true;
        }
    }
    if (changed)
        emit inputChanged();
}


bool DialogParameterView::convertToInput(int pnum) {
    return setInput(pnum);
/*    if (!pLabel.isEmpty()) {
        double value = sValue.at(pnum)->value();
        double oldValue = eInput.at(pnum)->value(), newValue = 0;
        QString txt = pType.at(pnum)->text();

        if (txt.contains("exp", Qt::CaseInsensitive)) {
            newValue = exp(value);
        }
        else { //if (txt.contains("value", Qt::CaseInsensitive)) {
            newValue = value;
        }
        if (newValue < oldValue || newValue > oldValue) {
            if (eInput.at(pnum)->minimum() > newValue)
                eInput.at(pnum)->setMinimum(newValue - 1.0);
            if (eInput.at(pnum)->maximum() < newValue)
                eInput.at(pnum)->setMaximum(newValue + 1.0);
            eInput.at(pnum)->setValue(newValue);
            emit dataChanged();
        }
    }*/
}

double DialogParameterView::getInput(int pnum) {
    double val = -1.0;
    if (eInput.count() > pnum) {
        val = eInput.at(pnum)->value();
    }
    return val;
}


void DialogParameterView::show() {
    resize(window);
    if (position.rx() != 0)
        move(position);
    update();
    setVisible(true);
    connect (parameters, SIGNAL(dataChanged()), SLOT(paramsChanged()));
}

void DialogParameterView::hide() {
    position = pos();
    window = size();
    setVisible(false);
    disconnect (parameters, SIGNAL(dataChanged()), this, SLOT(paramsChanged()));
}

void DialogParameterView::closeEvent(QCloseEvent *evt) {
    Q_UNUSED(evt);
    QDialog::hide();
    emit hidden();
}

void DialogParameterView::cancel() {
    close();
}

void DialogParameterView::reset() {
    setSliders();
    sValueChanged(0);
}

void DialogParameterView::apply() {
    disconnectAll();
    QStringList paramvalues;
    double val, min, max;
    QString label;
    for (int i = 0; i < numParamsShown; i++) {
        paramvalues = parameters->getRowData(i);
        label = pLabel.at(i)->text();
        min = pMin.at(i)->value();
        max = pMax.at(i)->value();
        val = sValue.at(i)->value();
        paramvalues[0] = QString::number(min);
        paramvalues[1] = QString::number(max);
        paramvalues[2] = QString::number(val);
        parameters->setRowData(i, paramvalues);
        parameters->setRowHeader(i, pLabel.at(i)->text());
        pInit.at(i)->setValue(val);
    }
    connectAll();
}

doubleLimitSpinBox *DialogParameterView::valueSpinBox(bool arrows, bool read) {
    doubleLimitSpinBox *vsb = new doubleLimitSpinBox(this);
    vsb->setFocusPolicy(Qt::StrongFocus);
    vsb->setValue(0);
    vsb->setDecimals(3);
    vsb->setSingleStep(.001);
    if (!arrows)
        vsb->setButtonSymbols(QAbstractSpinBox::NoButtons);
    if (!read)
        vsb->setReadOnly(true);
    return vsb;
}

QDoubleSpinBox *DialogParameterView::parameterSpinBox(bool arrows, bool read) {
    QDoubleSpinBox *psb = new QDoubleSpinBox(this);
    psb->setFocusPolicy(Qt::StrongFocus);
    psb->setRange(-200, 200);
    psb->setDecimals(3);
    psb->setSingleStep(.001);
    if (!arrows)
        psb->setButtonSymbols(QAbstractSpinBox::NoButtons);
    if (!read)
        psb->setReadOnly(true);
    return psb;
}

QSlider *DialogParameterView::parameterSlider() {
    QSlider *ps = new QSlider(Qt::Horizontal, this);
    ps->setRange(-20000, 20000);
    ps->setFocusPolicy(Qt::StrongFocus);
    return ps;
}

void DialogParameterView::buttonClicked(QAbstractButton* btn) {
    QString txt = btn->text();

    if      (txt.contains("Apply")) {
        apply();
    }
    else if (txt.contains("Reset")) {
        reset();
    }
    else {// if (btn->text().contains("Close")) {
        close();
    }
}

bool DialogParameterView::getShowTrans() const
{
    return transVisible;
}

void DialogParameterView::setShowTrans(bool value)
{
    transVisible = value;
    ui->labelParamType->setVisible(transVisible);
    ui->labelEquationInput->setVisible(transVisible);
}


template<typename T>
bool DialogParameterView::checkMinMax(T *min, T *max)
{
    bool changed = false;
    if (*min > *max) {
        T temp = *min;
        *min = *max;
        *max = temp;
        changed = true;
    }
    return changed;
}

