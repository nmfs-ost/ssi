#include "dialogselectivityequationview.h"

#include <QMessageBox>

using namespace tk;

#ifndef PRECISION
#define PRECISION .0005
#endif
#ifndef SLIDER_SCALE
#define SLIDER_SCALE 1000
#define VALUE_SCALE .001
#endif

#include <cmath>

static double neglog19 = -1.0 * log(19);


DialogSelexEquationView::DialogSelexEquationView(QWidget *parent, QString *typ)
 : DialogEquationView (parent)
{
    type = QString (*typ);
    name = QString (QString("%1 Selectivity").arg(type));
    title = name;

    selex = nullptr;
    fleet = nullptr;
    genders = 2;

    equationNum = -1;
    parameters = nullptr;

    setWindowTitle(name);
    parameterView->setTitle(name + QString(" Parameters"));
    equationNum = 0;
    numParams = 0;
    setIntVar1Range(1, 10);
    setIntVar1(2);

    yMax = 0;

    connect (this, SIGNAL(linearUpdated(float)), SLOT(updateLinearExp(float)));
}

DialogSelexEquationView::~DialogSelexEquationView()
{

}

Fleet *DialogSelexEquationView::getFleet() const {
    return fleet;
}

void DialogSelexEquationView::setFleet(Fleet *flt) {
    fleet = flt;
}

int DialogSelexEquationView::getSpecial() const
{
    return special;
}

void DialogSelexEquationView::setSpecial(int value)
{
    special = value;
}

void DialogSelexEquationView::setOption(int value)
{
    setEquationNumber(value);
}

void DialogSelexEquationView::setSelex(selectivity *slx)
{
    if (selex != nullptr &&
            selex->getNumXvals() > 0)
    {
        disconnect(selex, SIGNAL(dataChanged()), this, SLOT(changeSelex()));
        disconnect(selex, SIGNAL(startingSetupChanges()), this, SLOT(changingSelex()));
        disconnect(selex, SIGNAL(setupChanged(QStringList)), this, SLOT(changedSelex(QStringList)));
    }
    selex = slx;
    genders = selex->getNumGenders();
    connect(selex, SIGNAL(dataChanged()), this, SLOT(changeSelex()));
    connect(selex, SIGNAL(startingSetupChanges()), this, SLOT(changingSelex()));
    connect(selex, SIGNAL(setupChanged(QStringList)), this, SLOT(changedSelex(QStringList)));
    setParameters(selex->getParameterModel());
    changeSelex();
}

void DialogSelexEquationView::changingSelex()
{
    disconnect(parameters, SIGNAL(dataChanged()), this, SLOT(parametersChanged()));
}

void DialogSelexEquationView::changedSelex(QStringList ql)
{
    connect(parameters, SIGNAL(dataChanged()), this, SLOT(parametersChanged()));
    changeSelex();
}

void DialogSelexEquationView::changeSelex()
{
    restoreAll();
}

void DialogSelexEquationView::setXvals(const QList<float> &vals)
{
    setXvalsConst(vals);
/*    float val;
    if (!vals.isEmpty())
    {
        xValList.clear();
        for (int i = 0; i < vals.count(); i++)
        {
            val = vals.at(i);
            xValList.append(val);
        }
        ui->spinBox_bins_min->setValue(xValList.first());
        ui->spinBox_bins_max->setValue(xValList.last());
        ui->spinBox_bins_width->setValue(xValList.at(2) - xValList.at(1));
    }*/
}

void DialogSelexEquationView::setXvalStrings(const QStringList &vals)
{
    setXvalsConst(vals);
}

void DialogSelexEquationView::setParameterHeaders()
{
    QString fltNum(QString::number(fleetNum));
    QString fltTyp;
    Fleet::FleetType ft = fleet->getType();
    if     (ft == Fleet::Fishing ||
            ft == Fleet::Bycatch)
        fltTyp = QString("fishery");
    else if (ft == Fleet::Survey)
        fltTyp = QString("survey");

    for (int i = 0; i < parameters->rowCount(); i++)
    {
        parameters->setRowHeader(i, QString("%1Sel p%2 %3 (%4)").arg(
                     type, QString::number(i+1), fltTyp, fltNum));
    }
}

void DialogSelexEquationView::resetValues()
{

}

void DialogSelexEquationView::restoreAll()
{

}

void DialogSelexEquationView::parametersChanged()
{

}

void DialogSelexEquationView::setupChanged()
{

}

void DialogSelexEquationView::setup()
{
    if (fleet == nullptr)
        return;

    if (!updating)
    {
        updating = true;

        resetChart();
        if (type.compare(QString("Age"), Qt::CaseInsensitive) == 0)
            axisX->setTitleText(QString("Age Bins"));
        else
            axisX->setTitleText(QString("Length Bins"));
        axisY->setTitleText(QString("Biomass"));
        valSeries->setName("Catch");

        switch (equationNum)
        {
        case 0:  // case 0 constant size selectivity
        case 10: // 10 #Constant age-specific selex for ages 1 to nages
            numParams = 0;
            constant();
            break;

        case 11: // 11 #Constant age-specific selex for specified age range
            numParams = 2;
            constantRange();
            break;

        case 1:  // case 1 logistic size selectivity
        case 12: // 12 #age selectivity - logistic
            numParams = 2;
            logistic();
            break;

        case 2:  // case 2 discontinued; use pattern 8 for double logistic
        case 7:  // case 7 discontinued; use pattern 8 for double logistic
            numParams = 8;
            blank(8, 8);
            break;

        case 3:  // case 3 discontinued
            numParams = 6;
            blank (6, 0);
            break;

        case 13: // 13 #age selectivity - double logistic
            numParams = 8;
            dblLogPeak();
            break;

        case 4:  // case 4 discontinued; use pattern 30 to get spawning biomass
            numParams = 0;
            blank(0, 30);
            break;

        case 14: // 14 #age selectivity - separate parm for each age
            numParams = 0;
            eachAge();
            break;

        case 5:  // case 5 mirror another fleets size selectivity for specified bin range
            numParams = 2;
        case 15: // case 15 mirror another fleets size selectivity for all size bins
                 // 15 #age selectivity - mirror selex for lower numbered fleet
            mirror(numParams);
            break;

        case 6:  // case 6 non-parametric size selex pattern
            numParams = 2;
            linear(0);
            break;

        case 16: // case 16 Coleraine - Gaussian age selectivity
            numParams = 2;
            coleGauss();
            break;

        case 17: // 17 #age selectivity: each age has parameter as random walk
            //          transformation is selex=exp(parm); some special codes
            numParams = 0;
            randomWalk(0);
            break;

        case 8:  // case 8 double logistic with eight parameters
        case 18: // 18 #age selectivity: double logistic with smooth transition
            numParams = 8;
            dblLogPeak();
            break;

        case 9:  // case 9 old double logistic with 4 parameters
        case 19: // 19 #age selectivity: old double logistic
            numParams = 6;
            dblLogistic();
            break;

        case 21: // case 21 non-parametric size selectivity
            numParams = 0;
            linear(1);
            break;

        case 22: // case 22 size selectivity using double_normal_plateau (similar to CASAL)
            numParams = 4;
            dblNormCasal();
            break;

        case 20: // 20 #age selectivity: double normal with plateau
        case 23: // case 23 size selectivity double_normal_plateau where final value can be greater than 1.0
        case 24: // case 24 size selectivity using double_normal_plateau and lots of bells and whistles
            numParams = 6;
            dblNormEndpts();
            break;

        case 25: // case 25 size selectivity using exponential-logistic
        case 26: // 26 #age selectivity: exponential logistic
            numParams = 3;
            expLogistic();
            break;

        case 27: // 27 #age selectivity: cubic spline
            numParams = 3;
            cubicSpline();
            break;

        case 41: // 41 each age has parameter as random walk scaled by average of values at low age through high age
            //    transformation as selex=exp(parm); some special codes
            numParams = 2;
            randomWalk(2.);
            break;

        case 42: // 42 cubic spline scaled by average of values at low age through high age
            numParams = 5;
            cubicSpline(2.);
            break;

        case 43: // 43 non-parametric size selex scaled by average of values at low bin through high bin
            numParams = 4;
            linearScaled();
            break;

        case 44: // 44 like age selex 17 but with separate parameters for males and with revised controls
            numParams = genders + 2;
            twoSexRandomWalk();
            break;

        case 45: // 45 like age selex 14 but with separate parameters for males and with revised controls
            numParams = genders + 2;
            twoSexEachAge();
            break;

        default:
            numParams = 0;
            blank(0);
            break;

        }
        updating = false;
    }
    updateGrid(cht->rect());
}

void DialogSelexEquationView::refresh()
{
    setXvals(selex->getXVals());

    equationNum = selex->getPattern();
    special = selex->getSpecial();
    male = selex->getMale();
    // get values from parameters
    parameterView->reset();
}

void DialogSelexEquationView::update()
{
    if (fleet == nullptr)
        return;

    if (!updating)
    {
        updating = true;

        switch (equationNum)
        {
        case 0:  // case 0 constant size selectivity
        case 10: // 10 #Constant age-specific selex for ages 1 to nages
            updateConstant();
            break;

        case 11: // 11 #Constant age-specific selex for specified age range
            updateConstantRange();
            break;

        case 1:  // case 1 logistic size selectivity
        case 12: // 12 #age selectivity - logistic
            updateLogistic();
            break;

        case 5:  // case 5 mirror another fleets size selectivity for specified bin range
            updateMirror(2.);
            break;

        case 6:  // case 6 non-parametric size selex pattern
            updateLinear(0);
            break;

        case 8:  // case 8 double logistic with eight parameters
            updateDblLogPeak();
            break;

        case 18: // 18 #age selectivity: double logistic with smooth transition
            updateDblLogSmooth();
            break;

        case 9:  // case 9 old double logistic with 4 parameters
        case 19: // 19 #age selectivity: old double logistic
            updateDblLogistic();
            break;

        case 14: // 14 #age selectivity - separate parm for each age
            updateEachAge();
            break;

        case 16: // case 16 Coleraine - Gaussian age selectivity
            updateColeGauss();
            break;

        case 17: // 17 #age selectivity: each age has parameter as random walk
            //          transformation is selex=exp(parm); some special codes
            updateRandomWalk();
            break;

        case 20: // 20 #age selectivity: double normal with plateau
        case 24: // case 24 size selectivity using double_normal_plateau and lots of bells and whistles
            updateDblNormEndpts();
            break;

        case 21: // case 21 non-parametric size selectivity
            updateLinearPlain();
            break;

        case 22: // case 22 size selectivity using double_normal_plateau (similar to CASAL)
            updateDblNormCasal();
            break;

        case 23: // case 23 size selectivity double_normal_plateau where final value can be greater than 1.0
            updateDblNormPlateau();
            break;

        case 25: // case 25 size selectivity using exponential-logistic
        case 26: // 26 #age selectivity: exponential logistic
            updateExpLogistic();
            break;

        case 27: // 27 #age selectivity: cubic spline
            updateCubicSpline();
            break;

        case 41: // 41 each age has parameter as random walk scaled by average of values at low age through high age
            //    transformation as selex=exp(parm); some special codes
            updateRandomWalk(2.);
            break;

        case 42: // 42 cubic spline scaled by average of values at low age through high age
            updateCubicSpline(2.);
            break;

        case 43: // 43 non-parametric size selex scaled by average of values at low bin through high bin
            updateLinear(2.);
            break;

        case 44: // 44 like age selex 17 but with separate parameters for males and with revised controls
            updateTwoSexRandom();
            break;

        case 45: // 45 like age selex 14 but with separate parameters for males and with revised controls
            updateTwoSexEachAge();
            break;

        default:
            break;
        }
        updating = false;
    }
    updateGrid(cht->rect());

    QDialog::update();
}

// case 0 constant size selectivity
// 10 #Constant age-specific selex for ages 1 to nages
// Size selectivity 0, age selectivity 10 */
void DialogSelexEquationView::constant (float val)
{
    setTitle(QString("Pattern %1: Constant selectivity 1.0").arg(QString::number(equationNum)));
    parameters->setRowCount(0);
    showBins(true);
    showJoins(0);

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);

    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);

    updateConstant(val);
}

// Size selectivity 0, age selectivity 10
void DialogSelexEquationView::updateConstant (float val)
{
    float binMid = getBinStep() * getMidBin();
    float start = 0;
    float end = 1;

    if (!xValList.isEmpty())
    {
        if (equationNum == 0)
            start = xValList.at(0) + binMid;
        else
            start = xValList.at(1) + binMid;
    }
    end = xValList.last() + binMid;
    updateConstant(val, start, end);
}

// general constant graph
void DialogSelexEquationView::updateConstant (float val, float first, float last)
{
    float binMid = getBinStep() * getMidBin();
    float start = getBinMin() + binMid;
    float end = getBinMax() + binMid;

    valSeries->clear();
    if (equationNum == 0 || start >= first)
    {
        valSeries->append(QPointF(first, val));
    }
    else
    {
        valSeries->append(QPointF(start, 0));
        valSeries->append(QPointF(first, 0));
        valSeries->append(QPointF(first, val));
    }
    if (equationNum == 0 || end <= last)
    {
        valSeries->append(QPointF(end, val));
    }
    else
    {
        valSeries->append(QPointF(last, val));
        valSeries->append(QPointF(last, 0));
        valSeries->append(QPointF(end, 0));
    }
}

// case 11: Constant age-specific selex for specified age range
void DialogSelexEquationView::constantRange ()
{
    setTitle(QString("Pattern %1: Constant selectivity 1.0 within age range").arg(equationNum));
    parameters->setRowCount(2);
    connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));

    parameterView->setName(0, QString("Lo age"));
    parameterView->setType(0, QString("Value"));
    parameterView->setName(1, QString("Hi age"));
    parameterView->setName(1, QString("Value"));
    setParameterHeaders();
    showBins(true);
    showJoins(0);

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);
    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);

    if (parameters->rowCount() == 2)
        updateConstantRange();
}

// 11 #Constant age-specific selex for specified age range
void DialogSelexEquationView::updateConstantRange (float val)
{
    float par1 = parameterView->getInput(0);
    float par2 = parameterView->getInput(1);
    float binMid = getBinStep() * getMidBin();
    if (numParams < 2)
        return;
    if (par1 < xValList.first())
        par1 = xValList.first();
    if (par1 >= par2)
        par1 = par2 - binMid;
    if (par2 < xValList.first())
        par2 = xValList.last();
    if (par2 <= par1)
        par2 = par1 + binMid;
    float start = par1 + binMid;
    float end = par2 + binMid;

    updateConstant(val, start, end);
}

// case 1 logistic size selectivity
// 12 #age selectivity - logistic
// Size selectivity 1 and age selectivity 12
void DialogSelexEquationView::logistic ()
{
    setTitle(QString("Pattern %1: Logistic").arg(equationNum));
    disconnect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    parameterView->setName(0, QString("Value at 50%"));
    parameterView->setType(0, QString("Value"));
    parameterView->setName(1, QString("Diff 95% & 50%"));
    parameterView->setName(1, QString("Value"));
    setParameterHeaders();
    showBins(true);
    showJoins(0);

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);

    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);

    if (parameters->rowCount() == 2)
        updateLogistic();
}

// Size selectivity 1 and age selectivity 12
void DialogSelexEquationView::updateLogistic()
{
    float yVal = 0;
    float temp = 0;
    float len = 0;
    float par1 = parameterView->getInput(0);
    float par2 = parameterView->getInput(1);
    if (numParams < 2)
        return;
    if (par1 < 0)
        par1 = xValList.first();
    if (par2 < -50)
        par2 = -50;

    valSeries->clear();

    for (int i = 0; i < xValList.count(); i++)
    {
        len = xValList.at(i);
        temp = neglog19 * (len - par1) / par2;
        yVal = 1 / (1 + exp(temp));
        valSeries->append(QPointF(len, yVal));
    }
}

// case 5 mirror another fleets size selectivity for specified bin range
//   parm1 = lo bin; parm2 = hi bin
// case 15 mirror another fleets size selectivity for all size bins
// 15 #age selectivity - mirror selex for lower numbered fleet
void DialogSelexEquationView::mirror (int sliders)
{
    int flt = special;
    QString msg (QString("Pattern %1: Mirror of Fleet (%2)").arg(
                 QString::number(equationNum),
                 QString::number(flt)));
    if (flt >= fleet->getNumber())
    {
        msg.append(QString("\nMust use fleet (special) previously defined!"));
    }
    else
    {

    chartview->setVisible(false);
    if (sliders == 2)
    {
        if (numParams < 2)
            return;
        int num = xValList.count();
        disconnect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
        msg.append(QString(" between Lo and Hi bins"));
        connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
        parameterView->setName(0, QString("Lo bin"));
        parameterView->setType(0, QString("Value"));
        parameterView->setName(1, QString("Hi bin"));
        parameterView->setName(1, QString("Value"));
        setParameterHeaders();
    }
    }
    setTitle(msg);
    showBins (false);
    showJoins(0);

    updateMirror(sliders);
}

// size selectivity 5 and size and age selectivity 15
void DialogSelexEquationView::updateMirror(int sliders)
{
    int flt = special < 1? 1: special;
    if (fleetNum <= flt)
    {
        blank(equationNum, 0, tr("Mirror must be of previous fleet.\n Change Special value."));
    }
    else if (sliders == 2)
    {
        if (numParams < 2)
            return;
        QString msg;
        int start = 1, end = 1;
        float par1 = static_cast<int>(parameterView->getInput(0));
        float par2 = static_cast<int>(parameterView->getInput(1));
        if (par1 < 1){
            start = 1;
        }
        else {
            start = par1;
        }

        if (par2 < 1 || par2 > xValList.count()) {
            end = xValList.count();
        }
        else {
            end = par2;
        }

        if (end < start)
        {
            msg = QString ("Hi Bin is less than Lo Bin. adjusting Hi Bin ...");
            QMessageBox::information(this, tr("Error in Value"), tr(msg.toUtf8()));
            setSlider2(-1, xValList.count(), par1);
            return;
        }
        msg = QString (QString("Mirroring selectivity from Fleet %1 \nFrom Bin %2  to Bin %3").arg(
                           QString::number(getSpecial()),
                           QString::number(start),
                           QString::number(end)));
        ui->label_title->setText(msg);
    }
}

// size selectivity 6 and age selectivity 43
// case 21 non-parametric size selectivity
void DialogSelexEquationView::linear (float scale)
{
    int numNodes = getSpecial(), num = 0;
    QString msg(QString("Pattern %1: Non-parametric, %2 Linear segments").arg(
                    QString::number(equationNum),
                    QString::number(numNodes + 1)));
    setTitle(msg);

    if (scale == 1)
    {
        linearPlain ();
        return;
    }
    else if (scale == 2)
    {
        linearScaled();
        return;
    }

    num = parameters->rowCount() - 2;
    if (num < 2 || numNodes != num)
    {
        ui->label_title->setText(tr("Incorrect number of parameters. \nShould be Special +2"));
        ui->label_title->setVisible(true);
        return;
    }
    disconnect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    max1Changed(xValList.last()); min1Changed(xValList.first());
    max2Changed(xValList.last()); min2Changed(xValList.first());
    max3Changed(9); min3Changed(-5);
    max4Changed(9); min4Changed(-5);
    max5Changed(9); min5Changed(-5);
    max6Changed(9); min6Changed(-5);
    max7Changed(9); min7Changed(-5);
    max8Changed(9); min8Changed(-5);
    max9Changed(9); min9Changed(-5);
    connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));

    ui->label_1_name->setText("length for P1");
    ui->label_1_type->setText("Value");
    ui->label_2_name->setText("length for P2");
    ui->label_2_type->setText("Value");
    setParameterHeaders();
    if (numNodes > 7)
    {
        msg = QString("Non-parametric - too many segments(%1) to display!").arg(
                    QString::number(numNodes));
        blank(2, 0, msg);
        return;
    }
    else
    {
        showSliders(numNodes + 2);
        ui->label_3_name->setText("ln of sel at P1");
        ui->label_3_type->setText("Value");
        ui->label_4_name->setText("ln of sel midpt");
        ui->label_4_type->setText("Value");
        ui->label_5_name->setText("ln of sel midpt");
        ui->label_5_type->setText("Value");
        ui->label_6_name->setText("ln of sel midpt");
        ui->label_6_type->setText("Value");
        ui->label_7_name->setText("ln of sel midpt");
        ui->label_7_type->setText("Value");
        ui->label_8_name->setText("ln of sel midpt");
        ui->label_8_type->setText("Value");
        ui->label_9_name->setText("ln of sel at P2");
        ui->label_9_type->setText("Value");
//        setParameterHeaders();

        if (numNodes == 2)
        {
            ui->label_4_name->setText("ln of sel at P2");
        }
        else if (numNodes == 3)
        {
            ui->label_5_name->setText("ln of sel at P2");
        }
        else if (numNodes == 4)
        {
            ui->label_6_name->setText("ln of sel at P2");
        }
        else if (numNodes == 5)
        {
            ui->label_7_name->setText("ln of sel at P2");
        }
        else if (numNodes == 6)
        {
            ui->label_8_name->setText("ln of sel at P2");
        }
        showBins(true);
        showJoins(0);

//        resetChart();

        axisY->setTitleText("Recruit");
        cht->addAxis(axisY, Qt::AlignLeft);
        cht->addAxis(axisX, Qt::AlignBottom);

        ascendSeries = new QLineSeries(cht);
        ascendSeries->setPen(QPen(QBrush(Qt::blue), 2));
        ascendSeries->setName(QString("Ln of Selex"));
        cht->addSeries(ascendSeries);

        valSeries->setPen(QPen(QBrush(Qt::red), 3));
        cht->addSeries(valSeries);
        valSeries->attachAxis(axisX);
        valSeries->attachAxis(axisY);

        axisYalt->setTitleText("Ln of Selex");

        cht->addAxis(axisYalt, Qt::AlignRight);
        ascendSeries->attachAxis(axisX);
        ascendSeries->attachAxis(axisYalt);
        cht->legend()->show();
        cht->legend()->setAlignment(Qt::AlignLeft);
        updateLinear(scale);
    }
}
/* This is the code from SS_selex.tpl
 *    while(j<=nlength)
 *    {
 *      if(len_bins(j)<=sp(z))
 *      {
 *        sel(j) = lastsel + (len_bins(j)-lastSelPoint)/(sp(z)-lastSelPoint) * (sp(z+k)-lastsel);
 *        j++;
 *      }
 *      else if(z<=k)
 *      {
 *        lastSelPoint=sp(z);
 *        lastsel=sp(z+k);
 *        z++;
 *      }
 *      else  //  for sizes beyond last point
 *      {
 *        sel(j)=sp(k+k);
 *        j++;
 *      }
 *    }
 * I've simplified it to just the points given */
// case 21 non-parametric size selectivity
void DialogSelexEquationView::linearPlain()
{
    int numNodes = getSpecial(), num = 0;
    float min, max;
    QStringList rowdata;
    QString msg(QString("Pattern %1: Non-parametric %2 Linear segments").arg(
                    QString::number(equationNum),
                    QString::number(numNodes + 1)));
    setTitle(msg);

    num = numNodes * 2;
    if (parameters->rowCount() < num)
    {
        QMessageBox::information(this, tr("Parameter error"), tr("Incorrect number of parameters. \nShould be (Special * 2)"));
        return;
    }
    showSliders(0);
    showBins(true);
    showJoins(0);
//    resetChart();

    axisY->setTitleText("Selex");
    cht->addAxis(axisY, Qt::AlignLeft);
    cht->addAxis(axisX, Qt::AlignBottom);

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);
    valSeries->attachAxis(axisX);
    valSeries->attachAxis(axisY);

    setParameterHeaders();
    // set parameter min and max
    min = xValList.first();
    max = xValList.last();
    for (int i = 0; i < special; i++)
    {
        rowdata = parameters->getRowData(i);
        rowdata[0] = min; rowdata[1] = max;
        parameters->setRowData(i, rowdata);
    }
    updateLinearPlain();
}

// age selectivity 43
void DialogSelexEquationView::linearScaled ()
{
    int numNodes = getSpecial(), num = 0;
    QString msg(QString("Pattern %1: Non-parametric %2 Linear segments").arg(
                    QString::number(equationNum),
                    QString::number(numNodes + 1)));
    setTitle(msg);

    num = parameters->rowCount() - 4;
    if (num < 3 || numNodes != num)
    {
        QMessageBox::information(this, tr("Parameter error"), tr("Incorrect number of parameters. \nShould be Special +4"));
        return;
    }
    disconnect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    max1Changed(xValList.count()); min1Changed(1);
    max2Changed(xValList.count()); min2Changed(1);
    max3Changed(xValList.last()); min3Changed(xValList.first());
    max4Changed(xValList.last()); min4Changed(xValList.first());
    max5Changed(9); min5Changed(-5);
    max6Changed(9); min6Changed(-5);
    max7Changed(9); min7Changed(-5);
    max8Changed(9); min8Changed(-5);
    max9Changed(9); min9Changed(-5);
    connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));

    ui->label_1_name->setText("Scale bin Lo");
    ui->label_1_type->setText("Value");
    ui->label_2_name->setText("Scale bin Hi");
    ui->label_2_type->setText("Value");
    ui->label_3_name->setText("length for P1");
    ui->label_3_type->setText("Value");
    ui->label_4_name->setText("length for P2");
    ui->label_4_type->setText("Value");
    setParameterHeaders();
    if (numNodes > 5)
    {
        msg.append(" - too many segments to display!");
        blank(4, 0, msg);
    }
    else
    {
        showSliders(numNodes + 4);
        ui->label_5_name->setText("ln of sel at P1");
        ui->label_5_type->setText("Value");
        ui->label_6_name->setText("ln of sel midpt");
        ui->label_6_type->setText("Value");
        ui->label_7_name->setText("ln of sel midpt");
        ui->label_7_type->setText("Value");
        ui->label_8_name->setText("ln of sel midpt");
        ui->label_8_type->setText("Value");
        ui->label_9_type->setText("Value");
        setParameterHeaders();
        if (numNodes == 2)
        {
            ui->label_6_name->setText("ln of sel at P2");
        }
        else if (numNodes == 3)
        {
            ui->label_7_name->setText("ln of sel at P2");
        }
        else if (numNodes == 4)
        {
            ui->label_8_name->setText("ln of sel at P2");
        }
        else if (numNodes == 5)
        {
            ui->label_9_name->setText("ln of sel at P2");
        }
        showBins(true);
        showJoins(0);

//        resetChart();

        axisY->setTitleText("Selex");
        cht->addAxis(axisY, Qt::AlignLeft);
        cht->addAxis(axisX, Qt::AlignBottom);

        ascendSeries = new QLineSeries(cht);
        ascendSeries->setPen(QPen(QBrush(Qt::blue), 2));
        ascendSeries->setName(QString("Ln of Selex"));
        cht->addSeries(ascendSeries);

        valSeries->setPen(QPen(QBrush(Qt::red), 3));
        cht->addSeries(valSeries);
        valSeries->attachAxis(axisX);
        valSeries->attachAxis(axisY);

        axisYalt->setTitleText("Ln of Selex");

        cht->addAxis(axisYalt, Qt::AlignRight);
        ascendSeries->attachAxis(axisX);
        ascendSeries->attachAxis(axisYalt);
        cht->legend()->show();
        cht->legend()->setAlignment(Qt::AlignLeft);
        updateLinearScaled();
    }
}

// size selectivity 6
void DialogSelexEquationView::updateLinear(float scale)
{
    float y = 0;
    int axisYMax = 0;
    int axisYMin = 0;
    float temp = 0;
    float len = 0;
    int numNodes = getSpecial();
    float xVal[10];
    float yVal[10];
    int num = parameters->rowCount() - 2;

    if (scale > 0)
    {
        updateLinearScaled();
        return;
    }

    ascendSeries->clear();

    if (num < 2 || numNodes != num)
    {
        QMessageBox::information(this, tr("Parameter error"), tr("Incorrect number of parameters. \nShould be Special +2"));
        return;
    }
    if (numNodes > 7)
    {
        QString msg(QString("Pattern %1: %2 Linear segments - too many segments to display!").arg(
                        QString::number(equationNum),
                        QString::number(numNodes)));
        blank(2, 0, msg);
        return;
    }

    for (int i = 0; i < 10; i++)
    {
        xVal[i] = 0.0;
        yVal[i] = 0.0;
    }
    xVal[0] = xValList.first();
    yVal[0] = -10.0;
    xVal[1] = ui->doubleSpinBox_1_value->value();
    ui->doubleSpinBox_1_trans->setValue(xVal[1]);
    yVal[1] = ui->doubleSpinBox_3_value->value();
    ui->doubleSpinBox_3_trans->setValue(yVal[1]);
    yVal[2] = ui->doubleSpinBox_4_value->value();
    ui->doubleSpinBox_4_trans->setValue(yVal[2]);
    yVal[3] = ui->doubleSpinBox_5_value->value();
    ui->doubleSpinBox_5_trans->setValue(yVal[3]);
    yVal[4] = ui->doubleSpinBox_6_value->value();
    ui->doubleSpinBox_6_trans->setValue(yVal[4]);
    yVal[5] = ui->doubleSpinBox_7_value->value();
    ui->doubleSpinBox_7_trans->setValue(yVal[5]);
    yVal[6] = ui->doubleSpinBox_8_value->value();
    ui->doubleSpinBox_8_trans->setValue(yVal[6]);
    yVal[7] = ui->doubleSpinBox_9_value->value();
    ui->doubleSpinBox_9_trans->setValue(yVal[7]);

    xVal[numNodes] = ui->doubleSpinBox_2_value->value();
    ui->doubleSpinBox_2_trans->setValue(xVal[numNodes]);
    xVal[numNodes + 1] = xValList.last();
    yVal[numNodes + 1] = yVal[numNodes];

    temp = (xVal[numNodes] - xVal[1]) / (numNodes - 1);
    for (int i = 2; i < numNodes; i++)
    {
        xVal[i] = xVal[1] + (i - 1) * temp;
    }

    for (int i = 0; i <= numNodes; i++)
        ascendSeries->append(QPointF(xVal[i], yVal[i]));
    if (xVal[numNodes] > binMax)
        temp = xVal[numNodes] + 2;
    else
        temp = binMax;
    ascendSeries->append(QPointF(temp, yVal[numNodes]));

    firstPoints.clear();
    yMax = yVal[0];
    for (int i = 0; i < xValList.count(); i++)
    {
        len = xValList.at(i);
        for (int j = 1; j <= numNodes; j++)
        {
            if (len >= xVal[j -1] && len < xVal[j])
            {
                y = evaluateLine(QPointF(xVal[j-1], yVal[j-1]), QPointF(xVal[j], yVal[j]), len);
                if (y > yMax)
                    yMax = y;
                if (y < axisYMin)
                    axisYMin = abs(y) - 2;
                axisYMax = abs(yMax) + 1;
                firstPoints.append(QPointF(len, y));
                break;
            }
        }
        if (len >= xVal[numNodes])
        {
            y = yVal[numNodes];
            firstPoints.append(QPointF(len, y));
        }
    }
    axisYalt->setRange(axisYMin, axisYMax);
    updateLinearExp(scale);
}

// case 21 non-parametric size selectivity
void DialogSelexEquationView::updateLinearPlain()
{
    int i = 0;
    float xVal;
    float yVal;
    valSeries->clear();
    xVal = QString(parameters->getRowData(0).at(2)).toFloat();
    while (xVal > xValList.at(i))
    {
        valSeries->append(QPointF(xValList.at(i++), 0.0));
    }
    for (i = 0; i < special; i++)
    {
        xVal = QString(parameters->getRowData(i).at(2)).toFloat();
        yVal = QString(parameters->getRowData(i + special).at(2)).toFloat();
        valSeries->append(QPointF(xVal, yVal));
    }
    if (xVal < xValList.last())
        valSeries->append(QPointF(xValList.last(), yVal));
}

// age selectivity 43
void DialogSelexEquationView::updateLinearScaled()
{
    float y = 0;
    int axisYMax = 0;
    int axisYMin = 0;
    QString msg;
    float temp = 0;
    float len = 0;
    int numNodes = getSpecial();
    float xVal[10];
    float yVal[10];
    float xMin = ui->doubleSpinBox_1_value->value();
    float xMax = ui->doubleSpinBox_2_value->value();
    int num = parameters->rowCount() - 4;

    if (num < 2 || numNodes != num)
    {
        QMessageBox::information(this, tr("Parameter error"), tr("Incorrect number of parameters. \nShould be Special +4"));
        return;
    }

    if (xMin < 1)
    {
        xMin = 1;
//        msg = QString("Minimum for param 1 is first bin. adjusting ...");
//        QMessageBox::information(this, tr("Error in Value"), tr(msg.toUtf8()));
        value1Changed(xMin);
        return;
    }
    if (xMax < xMin)
    {
        xMax = xMin + 1.0;
//        msg = QString("Maximum is greater than Minimum. adjusting ...");
//        QMessageBox::information(this, tr("Error in Value"), tr(msg.toUtf8()));
        value2Changed(xMax);
        return;
    }
    if (xMax > xValList.count())
    {
        xMax = xValList.count();
//        msg = QString("Maximum for param 2 is last bin. adjusting ...");
//        QMessageBox::information(this, tr("Error in Value"), tr(msg.toUtf8()));
        value2Changed(xMax);
        return;
    }
    if (xMax < xMin)
    {
        xMin = xMax;
//        msg = QString("Minimum is greater than Maximum. adjusting ...");
//        QMessageBox::information(this, tr("Error in Value"), tr(msg.toUtf8()));
        value1Changed(xMin);
        return;
    }
    ui->doubleSpinBox_1_trans->setValue(xMin);
    ui->doubleSpinBox_2_trans->setValue(xMax);

    ascendSeries->clear();

    if (numNodes > 5)
    {
        QString msg(QString("Pattern %1: %2 Linear segments - too many segments to display!").arg(
                        QString::number(equationNum),
                        QString::number(numNodes)));
        blank(5, 0, msg);
        return;
    }

    for (int i = 0; i < 10; i++)
    {
        xVal[i] = 0.0;
        yVal[i] = 0.0;
    }
    xVal[0] = xValList.first();
    yVal[0] = -10.0;
    xVal[1] = ui->doubleSpinBox_3_value->value();
    ui->doubleSpinBox_1_trans->setValue(xVal[1]);
    yVal[1] = ui->doubleSpinBox_5_value->value();
    ui->doubleSpinBox_3_trans->setValue(yVal[1]);
    yVal[2] = ui->doubleSpinBox_6_value->value();
    ui->doubleSpinBox_4_trans->setValue(yVal[2]);
    yVal[3] = ui->doubleSpinBox_7_value->value();
    ui->doubleSpinBox_5_trans->setValue(yVal[3]);
    yVal[4] = ui->doubleSpinBox_8_value->value();
    ui->doubleSpinBox_6_trans->setValue(yVal[4]);
    yVal[5] = ui->doubleSpinBox_9_value->value();
    ui->doubleSpinBox_7_trans->setValue(yVal[5]);

    xVal[numNodes] = ui->doubleSpinBox_4_value->value();
    ui->doubleSpinBox_4_trans->setValue(xVal[numNodes]);
    xVal[numNodes + 1] = xValList.last();
    yVal[numNodes + 1] = yVal[numNodes];

    temp = (xVal[numNodes] - xVal[1]) / (numNodes - 1);
    for (int i = 2; i < numNodes; i++)
    {
        xVal[i] = xVal[1] + (i - 1) * temp;
    }

    for (int i = 0; i <= numNodes; i++)
        ascendSeries->append(QPointF(xVal[i], yVal[i]));
    if (xVal[numNodes] > binMax)
        temp = xVal[numNodes] + 2;
    else
        temp = binMax;
    ascendSeries->append(QPointF(temp, yVal[numNodes]));

    firstPoints.clear();
    yMax = yVal[0];
    for (int i = 0; i < xValList.count(); i++)
    {
        len = xValList.at(i);
        for (int j = 1; j <= numNodes; j++)
        {
            if (len >= xVal[j -1] && len < xVal[j])
            {
                y = evaluateLine(QPointF(xVal[j-1], yVal[j-1]), QPointF(xVal[j], yVal[j]), len);
                firstPoints.append(QPointF(len, y));
                if (y > yMax)
                    yMax = y;
                if (y < axisYMin)
                    axisYMin = (int)y - 2;
                break;
            }
        }
        if (len >= xVal[numNodes])
        {
            y = yVal[numNodes];
            firstPoints.append(QPointF(len, y));
        }
    }
    axisYMax = (int)yMax + 1;
    axisYalt->setRange(axisYMin, axisYMax);
    updateLinearExp(2);
}

void DialogSelexEquationView::updateLinearExp(float scale)
{
    float yVal;
    int first = ui->doubleSpinBox_1_trans->value();
    int last = ui->doubleSpinBox_2_trans->value();
    float yTotal = 1;
    float yAve = 1;

    valSeries->clear();

    if (scale == 0)
    {
        for (int i = 0; i < firstPoints.count(); i++)
        {
            yVal = exp(firstPoints.at(i).y() - yMax);
            valSeries->append(firstPoints.at(i).x(), yVal);
        }
    }
    else
    {
        for (int i = 0; i < firstPoints.count(); i++)
        {
            yVal = exp(firstPoints.at(i).y() - yMax);
            firstPoints[i].setY(yVal);
        }
        for (int i = first-1; i < last; i++)
        {
            yTotal += firstPoints.at(i).y();
        }
        yAve = yTotal / (last - first + 1);
        for (int i = 0; i < firstPoints.count(); i++)
        {
            yVal = firstPoints.at(i).y() / yAve;
            valSeries->append(firstPoints.at(i).x(), yVal);
        }
    }
}

float DialogSelexEquationView::evaluateLine(QPointF pt1, QPointF pt2, float x)
{
    float slope = (pt2.y() - pt1.y()) / (pt2.x() - pt1.x());
    float y = slope * (x - pt2.x()) + pt2.y();
    return y;
}

/** This is the code from ss_selex.tpl
 *     k1 = int(value(sp(5)));
 *     if(k1 > 1)
 *         sel(1,k1-1) = 0.0;
 *     sel(k1,nlength) = elem_prod((1/(1+mfexp(-sp(2)*(len_bins_m(k1,nlength)-sp(1)) ))),
 *                                 (1-1/(1+mfexp(-sp(4)*(len_bins_m(k1,nlength)-(sp(1)*sp(6)+sp(3))) ))) );
 *     sel += 1.0e-6;
 *     sel /= max(sel);
 */
// case 9 old double logistic with 4 parameters
// 19 #age selectivity: old double logistic
void DialogSelexEquationView::dblLogistic()
{
    setTitle(QString("Pattern %1: Simple Double Logistic").arg(
                     QString::number(equationNum)));
    disconnect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    max1Changed(xValList.last()); min1Changed(0);
    max2Changed(1); min2Changed(0);
    max3Changed(xValList.last()); min3Changed(xValList.first());
    max4Changed(0); min4Changed(-1);
    max5Changed(xValList.count()); min5Changed(1);
    max6Changed(1); min6Changed(0);
    connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    showSliders(6);
    ui->label_1_name->setText("Infl 1");
    ui->label_1_type->setText("Value");
    ui->label_2_name->setText("Slope 1");
    ui->label_2_type->setText("Value");
    ui->label_3_name->setText("Infl 2");
    ui->label_3_type->setText("Value");
    ui->label_4_name->setText("Slope 2");
    ui->label_4_type->setText("Value");
    ui->label_5_name->setText("First Bin");
    ui->label_5_type->setText("Value");
    ui->label_6_name->setText("Offset");
    ui->label_6_type->setText("Value");
    setParameterHeaders();
    showBins(true);
    showJoins(0);

//    resetChart();

    ascendSeries = new QLineSeries(cht);
    ascendSeries->setName(QString("Logist1"));
    ascendSeries->setPen(QPen(QBrush(Qt::green), 2));
    cht->addSeries(ascendSeries);

    dscendSeries = new QLineSeries(cht);
    dscendSeries->setName(QString("Logist2"));
    dscendSeries->setPen(QPen(QBrush(Qt::blue), 2));
    cht->addSeries(dscendSeries);

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);

    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);
//    cht->addAxis(axisYalt, Qt::AlignRight);

    ascendSeries->attachAxis(axisX);
    ascendSeries->attachAxis(axisY);

    dscendSeries->attachAxis(axisX);
    dscendSeries->attachAxis(axisY);
//    cht->addAxis(axisYalt, Qt::AlignRight);

    cht->legend()->setVisible(true);
    cht->legend()->setAlignment(Qt::AlignLeft);

    if (parameters->rowCount() == 6)
        updateDblLogistic();
}

void DialogSelexEquationView::updateDblLogistic()
{
    float maxSel = 0;
    float sel = 0;
    float term1 = 0, term2 = 0;
    float term1log, term2log;
    int limit = (xValList.count() - 1), i = 0, offset;
    float par1 = ui->doubleSpinBox_1_value->value();
    float par2 = ui->doubleSpinBox_2_value->value();
    float par3 = ui->doubleSpinBox_3_value->value();
    float par4 = ui->doubleSpinBox_4_value->value();
    int par5 = static_cast<int>(ui->doubleSpinBox_5_value->value());
    float par6 = (ui->doubleSpinBox_6_value->value());
    if (par1 < 0) par1 = 0;
    if (par2 < 0) par2 = 0;
    if (par3 < xValList.first()) par3 = xValList.first();
    if (par4 < -1) par4 = -1;

    limit = (par5 < 1)? 0: ((par5 < xValList.count())? (par5 - 1): limit);
    offset = (par6 < 0.5) ? 0: 1;
    ui->doubleSpinBox_1_trans->setValue(par1);
    ui->doubleSpinBox_2_trans->setValue(par2);
    ui->doubleSpinBox_3_trans->setValue(par3);
    ui->doubleSpinBox_4_trans->setValue(par4);
    ui->doubleSpinBox_5_trans->setValue(limit);
    ui->doubleSpinBox_6_trans->setValue(offset);

    ascendSeries->clear();
    dscendSeries->clear();
    valSeries->clear();
    firstPoints.clear();

    if (xValList.count() > 0)
    {
    for (i = 0; i < limit; i++)
    {
        term1 = par2 * (xValList.at(i) - par1);
        term2 = par4 * (xValList.at(i) - (par1 * offset + par3));
        term1log = logist(term1);
        term2log = logist(term2);
        ascendSeries->append(QPointF(xValList.at(i), term1log));
        dscendSeries->append(QPointF(xValList.at(i), term2log));
        firstPoints.append(QPointF(xValList.at(i), 0.0));
    }
    for (i = limit; i < xValList.count(); i++)
    {
        term1 = par2 * (xValList.at(i) - par1);
        term2 = par4 * (xValList.at(i) - (par1 * offset + par3));
        term1log = logist(term1);
        term2log = logist(term2);
        ascendSeries->append(QPointF(xValList.at(i), term1log));
        dscendSeries->append(QPointF(xValList.at(i), term2log));
        sel = term1log * term2log; //logist(term1) * logist(term2);
        firstPoints.append(QPointF(xValList.at(i), sel));
    }
    maxSel = maxYvalue(firstPoints);
    for (i = 0; i < firstPoints.count(); i++)
    {
        sel = (firstPoints.at(i).y() + .000001) / maxSel;
        valSeries->append(QPointF(xValList.at(i), sel));
    }
    maxSel = maxYvalue(dscendSeries->points());
    axisYalt->setRange(0, (maxSel * 1.2));
    }
}

// case 8 double logistic with eight parameters
// 13 #age selectivity - double logistic
// 18 #age selectivity: double logistic with smooth transition
// 1=peak, 2=init,  3=infl,  4=slope, 5=final, 6=infl2, 7=slope2
// 1=peak, 2=init,  3=infl,  4=slope, 5=final, 6=infl2, 7=slope2 8=binwidth
void DialogSelexEquationView::dblLogPeak()
{
    setTitle(QString("Pattern %1: Double Logistic with IF joins ").arg(
                 QString::number(equationNum)));
    disconnect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    max1Changed(xValList.last()); min1Changed(xValList.first());
    max2Changed(1); min2Changed(0);
    max3Changed(5); min3Changed(-5);
    max4Changed(3); min4Changed(-5);
    max5Changed(9); min5Changed(-5);
    max6Changed(9); min6Changed(-5);
    max7Changed(3); min7Changed(-5);
    max8Changed(20); min8Changed(1);
    connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    if (equationNum == 13)
        ui->label_title->setText(QString("This pattern is discouraged, use pattern #18"));
    showSliders(8);
    ui->label_1_name->setText("Peak");
    ui->label_1_type->setText("Value");
    ui->label_2_name->setText("Init");
    ui->label_2_type->setText("Value");
    ui->label_3_name->setText("Infl-Up");
    ui->label_3_type->setText("Logit");
    ui->label_4_name->setText("Slope-Up");
    ui->label_4_type->setText("Value");
    ui->label_5_name->setText("Final");
    ui->label_5_type->setText("Logit");
    ui->label_6_name->setText("Infl-Dn");
    ui->label_6_type->setText("Logit");
    ui->label_7_name->setText("Slope-Dn");
    ui->label_7_type->setText("Value");
    ui->label_8_name->setText("BinWidth");
    ui->label_8_type->setText("Value");
    setParameterHeaders();

    showBins(true);
    showJoins(3);

//    resetChart();

    join1Series = new QLineSeries(cht);
    join1Series->setPen(QPen(QBrush(Qt::darkGreen), 1));
    join1Series->setName(QString("Join1"));
    cht->addSeries(join1Series);

    join2Series = new QLineSeries(cht);
    join2Series->setPen(QPen(QBrush(Qt::darkCyan), 1));
    join2Series->setName(QString("Join2"));
    cht->addSeries(join2Series);

    join3Series = new QLineSeries(cht);
    join3Series->setPen(QPen(QBrush(Qt::darkYellow), 1));
    join3Series->setName(QString("Join3"));
    cht->addSeries(join3Series);

    ascendSeries = new QLineSeries(cht);
    ascendSeries->setPen(QPen(QBrush(Qt::green), 3));
    ascendSeries->setName(QString("Logist1"));
    cht->addSeries(ascendSeries);

    dscendSeries = new QLineSeries(cht);
    dscendSeries->setPen(QPen(QBrush(Qt::blue), 3));
    dscendSeries->setName(QString("Logist2"));
    cht->addSeries(dscendSeries);

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);

    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);

    ascendSeries->attachAxis(axisX);
    ascendSeries->attachAxis(axisY);
    dscendSeries->attachAxis(axisX);
    dscendSeries->attachAxis(axisY);
    join1Series->attachAxis(axisX);
    join1Series->attachAxis(axisY);
    join2Series->attachAxis(axisX);
    join2Series->attachAxis(axisY);
    join3Series->attachAxis(axisX);
    join3Series->attachAxis(axisY);

    cht->legend()->setVisible(true);
    cht->legend()->setAlignment(Qt::AlignLeft);

    if (parameters->rowCount() == 8)
    {
        if (equationNum == 8)
            updateDblLogPeak();
        else if (equationNum == 13)
            updateDblLogIf();
        else
            updateDblLogSmooth();
    }
}

// size selectivity 8
void DialogSelexEquationView::updateDblLogPeak()
{
    float maxSelX = 0;
    float minVal = binMin + (getBinStep() * getMidBin());
    float maxVal = binMax + (getBinStep() * getMidBin());
    float sel = 0;
    float t1 = 0, t1min, t1max, t1power;
    float t2 = 0, t2min, t2max, t2power;
    float t1range, t2range;
    float one_init, final_one;
    float jn1, jn2, jn3;
    float upselex, downselex;
    int binM, i;
    float par1 = ui->doubleSpinBox_1_value->value();
    float par2 = ui->doubleSpinBox_2_value->value();
    float par3 = ui->doubleSpinBox_3_value->value();
    float par4 = ui->doubleSpinBox_4_value->value();
    float par5 = ui->doubleSpinBox_5_value->value();
    float par6 = ui->doubleSpinBox_6_value->value();
    float par7 = ui->doubleSpinBox_7_value->value();
    float par8 = ui->doubleSpinBox_8_value->value();
    if (par1 < min1) par1 = min1;
    if (par2 < min2) par2 = min2;
    if (par3 < min3) par3 = min3;
    if (par4 < min4) par4 = min4;
    if (par5 < min5) par5 = min5;
    if (par6 < min6) par6 = min6;
    if (par7 < min7) par7 = min7;
    if (par8 < min8) par8 = min8;
    float peak = par1;
    float init = par2;
    float infl_up = logist(par3);
    float slope_up = par4;
    float final = logist(par5);
    float infl_dn = logist(par6);
    float slope_dn = par7;
    float binwid = par8;
    ui->doubleSpinBox_1_trans->setValue(peak);
    ui->doubleSpinBox_2_trans->setValue(init);
    ui->doubleSpinBox_3_trans->setValue(infl_up);
    ui->doubleSpinBox_4_trans->setValue(slope_up);
    ui->doubleSpinBox_5_trans->setValue(final);
    ui->doubleSpinBox_6_trans->setValue(infl_dn);
    ui->doubleSpinBox_7_trans->setValue(slope_dn);
    ui->doubleSpinBox_8_trans->setValue(binwid);

    valSeries->clear();
    firstPoints.clear();
    ascendSeries->clear();
    dscendSeries->clear();
    join1Series->clear();
    join2Series->clear();
    join3Series->clear();
    float peakBwd = peak + binwid;
//    t1=minL+(1./(1.+mfexp(-sp(3))))*(sp(1)-minL);    // INFL
//    t1min=1./(1.+mfexp(-mfexp(sp(4))*(minL-t1)))*0.9999;  // asc value at minsize
//    t1max=1./(1.+mfexp(-mfexp(sp(4))*(sp(1)-t1)))*1.0001;  // asc value at peak
//    t1power=log(0.5)/log((0.5-t1min)/(t1max-t1min));  // so the parameter will actual correspond to 50% point
    t1 = minVal + (infl_up * (peak - minVal));
    t1min = logist (exp(slope_up) * (minVal - t1)) * 0.9999;// asc value at minsize
    t1max = logist (exp(slope_up) * (peak - t1)) * 1.0001;  // asc value at peak
    t1power = log(0.5)/log((0.5 - t1min)/(t1max - t1min));  // so the parameter will actual correspond to 50% point

//    if(seltype(f,4)==0) {sel_maxL=maxL;} else {sel_maxL=Ave_Size(styr,3,1,nages);}
    if (special == 0)
    {
        maxSelX = maxVal;
    }
    else
    {
        maxSelX = aveXvalue(xValList);
    }
//    t2=(sp(1)+sp(8))+(1./(1.+mfexp(-sp(6))))*(sel_maxL-(sp(1)+sp(8)));    // INFL
//    t2min=1./(1.+mfexp(-mfexp(sp(7))*(sp(1)+sp(8)-t2)))*0.9999;  // asc value at peak+
//    t2max=1./(1.+mfexp(-mfexp(sp(7))*(sel_maxL-t2)))*1.0001;  // asc value at maxL
//    t2power=log(0.5)/log((0.5-t2min)/(t2max-t2min));
    t2 = peakBwd + infl_dn * (maxSelX - peakBwd);    // INFL
    t2min = logist(exp(slope_dn) * (peakBwd - t2)) * 0.9999;// asc value at peak+
    t2max = logist(exp(slope_dn) * (maxSelX - t2)) * 1.0001;// asc value at maxL
    t2power = log(0.5) / log((0.5 - t2min)/(t2max - t2min));

/*    for (j=1; j<=nlength; j++)  //calculate the value over length bins
    {   join1=1./(1.+mfexp(10.*(len_bins_m(j)-sp(1))));
        join2=1./(1.+mfexp(10.*(len_bins_m(j)-(sp(1)+sp(8)))));
        join3=1./(1.+mfexp(10.*(len_bins_m(j)-sel_maxL)));
        upselex=sp(2) + (1. - sp(2)) * pow((( 1./(1.+mfexp(-mfexp(sp(4))*(len_bins_m(j)-t1)))-t1min ) / (t1max-t1min)),t1power);
        downselex=(1. + (final - 1.) * pow(fabs(((( 1./(1.+mfexp(-mfexp(sp(7))*(len_bins_m(j)-t2))) -t2min ) / (t2max-t2min) ))),t2power));
        sel(j) = ((((upselex*join1)+1.0*(1.0-join1))*join2) + downselex*(1-join2))*join3 + final*(1-join3);
    }*/
    one_init = 1.0 - init;
    final_one = final - 1.0;
    t1range = t1max - t1min;
    t2range = t2max - t2min;

    for (i = 0; i < xValList.count(); i++)
    {
        binM = xValList.at(i) + getBinStep() * getMidBin();
        jn1 = logist(-join1 * (binM - peak));
        jn2 = logist(-join2 * (binM - peakBwd));
        jn3 = logist(-join3 * (binM - maxVal));
//        jn1 = 1.0 / (1.0 + exp(10.0 * (binM - peak)));
//        jn2 = 1.0 / (1.0 + exp(10.0 * (binM - peakBwd)));
//        jn3 = 1.0 / (1.0 + exp(10.0 * (binM - maxSelX)));
        upselex = init + (one_init) * pow(((logist(exp(slope_up) * (binM - t1)) - t1min) / (t1range)), t1power);
        downselex = (1.0 + (final_one) * pow(fabs((((logist(exp(slope_dn) * (binM - t2)) -t2min) / (t2range)))), t2power));
//        upselex = init + (1.0 - init) * pow((( 1.0 / (1.0 + exp(-exp(slope_up) * (binM - t1))) - t1min) / (t1max - t1min)), t1power);
//        downselex = (1.0 + (final - 1.0) * pow(abs((((1.0 / (1.0 + exp(-exp(slope_dn) * (binM - t2))) -t2min) / (t2max - t2min)))), t2power));
        sel = ((((upselex * jn1) + (1.0 - jn1)) * jn2) + downselex * (1.0 - jn2)) * jn3 + final * (1.0 - jn3);
//        sel = ((upselex * jn1) + (jn2 - jn1) + (downselex * (jn3 - jn2)) + (final * (1.0 - jn3)));
        join1Series->append(QPointF(binM, jn1));
        join2Series->append(QPointF(binM, jn2));
        join3Series->append(QPointF(binM, jn3));
        ascendSeries->append(QPointF(binM, upselex));
        dscendSeries->append(QPointF(binM, downselex));
        valSeries->append(QPointF(binM, sel));
    }
//    ui->spinBox_steep_join1->setValue(10);
//    ui->spinBox_steep_join2->setValue(10);
//    ui->spinBox_steep_join3->setValue(10);
}

// 13 #age selectivity - double logistic
void DialogSelexEquationView::updateDblLogIf()
{
    float maxSelX = 0;
    float minX = xValList.first();
    float maxX = xValList.last();
    float sel = 0;
    float t1 = 0, t1min, t1max, t1power;
    float t2 = 0, t2min, t2max, t2power;
    float jn1, jn2;
    float upsel, dnsel;
    int binM, i;
    float par1 = ui->doubleSpinBox_1_value->value();
    float par2 = ui->doubleSpinBox_2_value->value();
    float par3 = ui->doubleSpinBox_3_value->value();
    float par4 = ui->doubleSpinBox_4_value->value();
    float par5 = ui->doubleSpinBox_5_value->value();
    float par6 = ui->doubleSpinBox_6_value->value();
    float par7 = ui->doubleSpinBox_7_value->value();
    float par8 = ui->doubleSpinBox_8_value->value();
    if (par1 < min1) par1 = min1;
    if (par2 < min2) par2 = min2;
    if (par3 < min3) par3 = min3;
    if (par4 < min4) par4 = min4;
    if (par5 < min5) par5 = min5;
    if (par6 < min6) par6 = min6;
    if (par7 < min7) par7 = min7;
    if (par8 < min8) par8 = min8;
    float peak = par1;
    float init = par2;
    float infl_up = logist(par3);
    float slope_up = par4;
    float final = logist(par5);
    float infl_dn = logist(par6);
    float slope_dn = par7;
    float binwid = par8;
    ui->doubleSpinBox_1_trans->setValue(peak);
    ui->doubleSpinBox_2_trans->setValue(init);
    ui->doubleSpinBox_3_trans->setValue(infl_up);
    ui->doubleSpinBox_4_trans->setValue(slope_up);
    ui->doubleSpinBox_5_trans->setValue(final);
    ui->doubleSpinBox_6_trans->setValue(infl_dn);
    ui->doubleSpinBox_7_trans->setValue(slope_dn);
    ui->doubleSpinBox_8_trans->setValue(binwid);

    valSeries->clear();
    firstPoints.clear();
    ascendSeries->clear();
    dscendSeries->clear();
    join1Series->clear();
    join2Series->clear();
    join3Series->clear();

    float peakBwd = peak + binwid;

}

void DialogSelexEquationView::updateDblLogSmooth()
{
    float maxSelX = 0;
    float minX = xValList.first();
    float maxX = xValList.last();
    float sel = 0;
    float t1 = 0, t1min, t1max, t1power;
    float t2 = 0, t2min, t2max, t2power;
    float jn1, jn2;
    float upsel, dnsel;
    int binM, i;
    float par1 = ui->doubleSpinBox_1_value->value();
    float par2 = ui->doubleSpinBox_2_value->value();
    float par3 = ui->doubleSpinBox_3_value->value();
    float par4 = ui->doubleSpinBox_4_value->value();
    float par5 = ui->doubleSpinBox_5_value->value();
    float par6 = ui->doubleSpinBox_6_value->value();
    float par7 = ui->doubleSpinBox_7_value->value();
    float par8 = ui->doubleSpinBox_8_value->value();
    if (par1 < min1) par1 = min1;
    if (par2 < min2) par2 = min2;
    if (par3 < min3) par3 = min3;
    if (par4 < min4) par4 = min4;
    if (par5 < min5) par5 = min5;
    if (par6 < min6) par6 = min6;
    if (par7 < min7) par7 = min7;
    if (par8 < min8) par8 = min8;
    float peak = par1;
    float init = par2;
    float infl_up = logist(par3);
    float slope_up = par4;
    float final = logist(par5);
    float infl_dn = logist(par6);
    float slope_dn = par7;
    float binwid = par8;
    ui->doubleSpinBox_1_trans->setValue(peak);
    ui->doubleSpinBox_2_trans->setValue(init);
    ui->doubleSpinBox_3_trans->setValue(infl_up);
    ui->doubleSpinBox_4_trans->setValue(slope_up);
    ui->doubleSpinBox_5_trans->setValue(final);
    ui->doubleSpinBox_6_trans->setValue(infl_dn);
    ui->doubleSpinBox_7_trans->setValue(slope_dn);
    ui->doubleSpinBox_8_trans->setValue(binwid);

    valSeries->clear();
    firstPoints.clear();
    ascendSeries->clear();
    dscendSeries->clear();
    join1Series->clear();
    join2Series->clear();
    join3Series->clear();

    float peakBwd = peak + binwid;
//    t1=0.+(1./(1.+mfexp(-sp(3))))*(sp(1)-0.);    // INFL
//    t1min=1./(1.+mfexp(-sp(4)*(0.-t1)))*0.9999;  // asc value at minsize
//    t1max=1./(1.+mfexp(-sp(4)*(sp(1)-t1)))*1.00001;  // asc value at peak
//    t1power=log(0.5)/log((0.5-t1min)/(t1max-t1min));
    t1 = 0.0 + (infl_up * (peak - 0.0));
    t1min = logist(exp(slope_up) * (0.0 - t1)) * 0.9999;
    t1max = logist(exp(slope_up) * (peak - t1)) * 1.0001;
//    t1min = 1.0/(1.0 + exp(-exp(par4) * (0.0 - t1))) * 0.9999;
//    t1max = 1.0/(1.0 + exp(-exp(par4) * (par1 - t1))) * 1.0001;
    t1power = log(0.5)/log((0.5 - t1min)/(t1max - t1min));


//    t2=(sp(1)+sp(8))+(1./(1.+mfexp(-sp(6))))*(r_ages(nages)-(sp(1)+sp(8)));    // INFL
//    t2min=1./(1.+mfexp(-sp(7)*(sp(1)+sp(8)-t2)))*0.9999;  // asc value at peak+
//    t2max=1./(1.+mfexp(-sp(7)*(r_ages(nages)-t2)))*1.00001;  // asc value at maxage
//    t2power=log(0.5)/log((0.5-t2min)/(t2max-t2min));
    t2 = (peakBwd) + infl_dn * (binMax - (peakBwd));
    t2min = logist(exp(slope_dn) * (peakBwd - t2)) * 0.9999;
    t2max = logist(exp(slope_dn) * (binMax - t2)) * 1.0001;
 //   t2min = 1./(1. + exp(-exp(par7) * (peak + par8 - t2))) * 0.9999;
//    t2max = 1./(1. + exp(-exp(par7) * (binMax - t2))) * 1.0001;
    t2power = log(0.5) / log((0.5 - t2min)/(t2max - t2min));
//    final=1./(1.+mfexp(-sp(5)));
//    final = logist(par5); // done above

/*    for (a=0; a<=nages; a++)  //calculate the value over ages
    {
        sel_a(y,fs,1,a) =
        (
        (
        (sp(2) + (1.-sp(2)) *
        pow((( 1./(1.+mfexp(-sp(4)*(r_ages(a)-t1)))-t1min)/ (t1max-t1min)),t1power))
        /(1.0+mfexp(30.*(r_ages(a)-sp(1))))  // scale ascending side
        +
        1./(1.+mfexp(-30.*(r_ages(a)-sp(1))))   // flattop, with scaling
        )
        /(1.+mfexp( 30.*(r_ages(a)-(sp(1)+sp(8)))))    // scale combo of ascending and flattop
        +
        (1. + (final - 1.) *
        pow(fabs((( 1./(1.+mfexp(-sp(7)*(r_ages(a)-t2))) -t2min ) / (t2max-t2min) )),t2power))
        /(1.+mfexp( -30.*(r_ages(a)-(sp(1)+sp(8)))))    // scale descending
        );
    }   // end age loop */
    for (i = 0; i < xValList.count(); i++)
    {
        binM = xValList.at(i);
        jn1 = logist (-join1 * (binM - peak));
        jn2 = logist (-join2 * (binM - peakBwd));
//        jn1 = 1.0 / (1.0 + exp(30.0 * (binM - peak)));
  //      jn2 = 1.0 / (1.0 + exp(30.0 * (binM - peakBwd)));
        upsel = init + (1.0 - init) * pow((( 1.0 / (1.0 + exp(-slope_up * (binM - t1))) - t1min) / (t1max - t1min)), t1power);
        dnsel = (1.0 + (final - 1.0) * pow(abs((((1.0 / (1.0 + exp(-slope_dn * (binM - t2))) -t2min) / (t2max - t2min)))), t2power));
        join1Series->append(QPoint(binM, jn1));
        join2Series->append(QPointF(binM, jn2));
//        join3Series->append(QPointF(binM, jn3));
        ascendSeries->append(QPointF(binM, upsel));
        dscendSeries->append(QPointF(binM, dnsel));
        sel = ((((upsel * jn1) + jn1) * jn2) + dnsel * jn2);
        sel = (upsel * jn1) + (jn2 - jn1) + (dnsel * (1.0 - jn2));
        valSeries->append(QPointF(binM, sel));
    }
}

/** Case 22: size selectivity using double normal plateau
 *  (Similar to CASAL)
 *
 * peak2=sp(1)+ (0.99*len_bins(nlength)-sp(1))/(1.+mfexp(-sp(2)));
 * upselex=mfexp(sp(3));
 * downselex=mfexp(sp(4));
 * for (j=1;j<=nlength;j++)
 * {
 *     t1=len_bins_m(j);
 *     if(t1<sp(1))
 *     {
 *         sel(j)=mfexp(-square(t1-sp(1))/upselex);
 *     }
 *     else if (t1<=peak2)
 *     {
 *         sel(j)=1.0;
 *     }
 *     else
 *     {
 *         sel(j)=mfexp(-square(t1-peak2)/downselex);
 *     }
 * }
*/
void DialogSelexEquationView::dblNormCasal()
{
    setTitle(QString("Pattern %1: Double Normal (like CASAL)").arg(
                 QString::number(equationNum)));
    disconnect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    max1Changed(xValList.last()); min1Changed(xValList.first());
    max2Changed(1); min2Changed(0);
    max3Changed(10); min3Changed(0);
    max4Changed(10); min4Changed(0);
    connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    showSliders(4);
    ui->label_1_name->setText("Peak");
    ui->label_1_type->setText("Value");
    ui->label_2_name->setText("Top");
    ui->label_2_type->setText("Log");
    ui->label_3_name->setText("Asc-width");
    ui->label_3_type->setText("Exp");
    ui->label_4_name->setText("Dsc-width");
    ui->label_4_type->setText("Exp");
    setParameterHeaders();

    showBins(true);
    showJoins(0);

//    resetChart();

    ascendSeries = new QLineSeries(cht);
    ascendSeries->setPen(QPen(QBrush(Qt::green), 2));
    ascendSeries->setName(QString("Ascend"));
    cht->addSeries(ascendSeries);
    ascendSeries->attachAxis(axisX);

    dscendSeries = new QLineSeries(cht);
    dscendSeries->setPen(QPen(QBrush(Qt::blue), 2));
    dscendSeries->setName(QString("Descend"));
    cht->addSeries(dscendSeries);
    dscendSeries->attachAxis(axisX);

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);

    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);

    ascendSeries->attachAxis(axisX);
    ascendSeries->attachAxis(axisY);
    dscendSeries->attachAxis(axisX);
    dscendSeries->attachAxis(axisY);

    cht->legend()->setVisible(true);
    cht->legend()->setAlignment(Qt::AlignLeft);

    if (parameters->rowCount() == 4)
        updateDblNormCasal();
}

void DialogSelexEquationView::updateDblNormCasal()
{
    float xval, sel, peak2;
    float par1 = ui->doubleSpinBox_1_value->value();
    float par2 = ui->doubleSpinBox_2_value->value();
    float par3 = ui->doubleSpinBox_3_value->value();
    float par4 = ui->doubleSpinBox_4_value->value();
    if (par1 < min1) par1 = min1;
    if (par2 < min2) par2 = min2;
    if (par3 < min3) par3 = min3;
    if (par4 < min4) par4 = min4;
    float peak = par1;
    float top = exp(-par2);
    float upsel = exp(par3);
    float dnsel = exp(par4);
    ui->doubleSpinBox_1_trans->setValue(peak);
    ui->doubleSpinBox_2_trans->setValue(top);
    ui->doubleSpinBox_3_trans->setValue(upsel);
    ui->doubleSpinBox_4_trans->setValue(dnsel);

    peak2 = peak + (0.99 * xValList.last() - peak) / (1. + top);

    valSeries->clear();
    ascendSeries->clear();
    dscendSeries->clear();
    for (int i = 0; i < xValList.count(); i++)
    {
        xval = xValList.at(i);
        sel = exp(-pow((xval - peak),2)/upsel);
        ascendSeries->append(QPointF(xval, sel));

        sel = exp(-pow((xval - peak2),2)/dnsel);
        dscendSeries->append(QPointF(xval, sel));
    }
    for (int i = 0; i < xValList.count(); i++)
    {
        xval = xValList.at(i);
        if (xval < peak)
        {
            sel = ascendSeries->points().at(i).y();
//            sel = exp(-pow((xval - peak),2)/upsel);
        }
        else if (xval <= peak2)
        {
            sel = 1.0;
        }
        else
        {
            sel = dscendSeries->points().at(i).y();
//            sel = exp(-pow((xval - peak2),2)/dnsel);
        }
        valSeries->append(QPointF(xval, sel));
    }
}

void DialogSelexEquationView::dblNormal()
{
    setTitle(QString("Pattern %1: Double Normal").arg(
                 QString::number(equationNum)));
    disconnect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    max1Changed(xValList.last()); min1Changed(xValList.first());
    max2Changed(1); min2Changed(0);
    max3Changed(10); min3Changed(0);
    max4Changed(10); min4Changed(0);
    connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    showSliders(4);
    ui->label_1_name->setText("Peak");
    ui->label_1_type->setText("Value");
    ui->label_2_name->setText("Top");
    ui->label_2_type->setText("Peak+Log");
    ui->label_3_name->setText("Asc-width");
    ui->label_3_type->setText("Exp");
    ui->label_4_name->setText("Dsc-width");
    ui->label_4_type->setText("Exp");
    setParameterHeaders();

    showBins(true);
    showJoins(2);

//    resetChart();

    join1Series = new QLineSeries(cht);
    join1Series->setPen(QPen(QBrush(Qt::darkYellow), 2));
    join1Series->setName(QString("Join1"));
    cht->addSeries(join1Series);

    join2Series = new QLineSeries(cht);
    join2Series->setPen(QPen(QBrush(Qt::yellow), 2));
    join2Series->setName(QString("Join2"));
    cht->addSeries(join2Series);

    ascendSeries = new QLineSeries(cht);
    ascendSeries->setPen(QPen(QBrush(Qt::green), 3));
    ascendSeries->setName(QString("Asc-Norm"));
    cht->addSeries(ascendSeries);

    dscendSeries = new QLineSeries(cht);
    dscendSeries->setPen(QPen(QBrush(Qt::blue), 3));
    dscendSeries->setName(QString("Desc-Norm"));
    cht->addSeries(dscendSeries);

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);

    cht->setAxisX(axisX, valSeries);
    cht->setAxisY(axisY, valSeries);

    ascendSeries->attachAxis(axisX);
    ascendSeries->attachAxis(axisY);
    dscendSeries->attachAxis(axisX);
    dscendSeries->attachAxis(axisY);
    join1Series->attachAxis(axisX);
    join1Series->attachAxis(axisY);
    join2Series->attachAxis(axisX);
    join2Series->attachAxis(axisY);

    cht->legend()->setVisible(true);
    cht->legend()->setAlignment(Qt::AlignLeft);

    if (parameters->rowCount() == 4)
        updateDblNormal();
}

void DialogSelexEquationView::updateDblNormal()
{
    float xval, sel, peak2;
    float par1 = ui->doubleSpinBox_1_value->value();
    float par2 = ui->doubleSpinBox_2_value->value();
    float par3 = ui->doubleSpinBox_3_value->value();
    float par4 = ui->doubleSpinBox_4_value->value();
    if (par1 < min1) par1 = min1;
    if (par2 < min2) par2 = min2;
    if (par3 < min3) par3 = min3;
    if (par4 < min4) par4 = min4;
    float peak = par1;
    float top = exp(-par2);
    float upsel = exp(par3);
    float dnsel = exp(par4);
}

void DialogSelexEquationView::dblNormPlateau()
{
    setTitle(QString("Pattern %1: Double Normal plateau").arg(
                 QString::number(equationNum)));
    disconnect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    max1Changed(xValList.last()); min1Changed(xValList.first());
    max2Changed(1); min2Changed(0);
    max3Changed(10); min3Changed(0);
    max4Changed(10); min4Changed(0);
    max5Changed(100); min5Changed(0);
    showSliders(6);
    ui->label_1_name->setText("Peak");
    ui->label_1_type->setText("Value");
    ui->label_2_name->setText("Top");
    ui->label_2_type->setText("Peak+Log");
    ui->label_3_name->setText("Asc-width");
    ui->label_3_type->setText("Exp");
    ui->label_4_name->setText("Dsc-width");
    ui->label_4_type->setText("Exp");
    ui->label_5_name->setText("Initial");
    ui->label_5_type->setText("Log");
    ui->label_6_name->setText("Final");
    if(equationNum == 23)
    {
        max6Changed(1); min6Changed(0);
        ui->label_6_type->setText("Value");
    }
    else
    {
        max6Changed(100); min6Changed(0);
        ui->label_6_type->setText("Log");
    }
    connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    setParameterHeaders();

    showBins(true);
    showJoins(2);

//    resetChart();

    updateDblNormPlateau();
}

void DialogSelexEquationView::updateDblNormPlateau()
{
    float par1 = ui->doubleSpinBox_1_value->value();
    float par2 = ui->doubleSpinBox_2_value->value();
    float par3 = ui->doubleSpinBox_3_value->value();
    float par4 = ui->doubleSpinBox_4_value->value();
    float par5 = ui->doubleSpinBox_5_value->value();
    float par6 = ui->doubleSpinBox_6_value->value();
    if (par1 < min1) par1 = min1;
    if (par2 < min2) par2 = min2;
    if (par3 < min3) par3 = min3;
    if (par4 < min4) par4 = min4;
    if (par5 < min5) par5 = min5;
    if (par6 < min6) par6 = min6;

    float peak = par1;
    float peak2 = peak+binWidth + (.99*binMax - peak - binWidth) * logist(par2);
    float upsel = exp(par3);
    float dnsel = exp(par4);
    float init = 0;//logist(par5);
    float final = (par6);
    float t1, t1min, t1term;
    float t2, t2min, t2term;
    float apical_selex = 1.0;
    int first, last;
    float jn1, jn2;
    int i;
    float xval = 0, asc = 0, des = 0, sel = 0;

    ui->doubleSpinBox_1_trans->setValue(peak);
    ui->doubleSpinBox_2_trans->setValue(peak2);
    ui->doubleSpinBox_3_trans->setValue(upsel);
    ui->doubleSpinBox_4_trans->setValue(dnsel);
    ui->doubleSpinBox_5_trans->setValue(init);
    ui->doubleSpinBox_6_trans->setValue(final);

    firstPoints.clear();
    ascendSeries->clear();
    dscendSeries->clear();
    join1Series->clear();
    join2Series->clear();
    valSeries->clear();

    if (xValList.count() > 0)
    {
        if (par5 < -1000)
        {
            first = -1000 - static_cast<int>(par5);
            init = 0;
            ui->doubleSpinBox_5_trans->setValue(first);
            t1min = 0;
        }
        else if (par5 > -999)
        {
            first = 0;
            init = logist(par5);
            ui->doubleSpinBox_5_trans->setValue(init);
            t1min = exp(-1 * (pow((xValList.at(first)-peak), 2)/upsel));
        }
        if (par6 < -1000)
        {
            last = -1000 - static_cast<int>(par6);
            final = 0;
            ui->doubleSpinBox_6_trans->setValue(last);
            t2min = 0;
        }
        else if (par6 > -999)
        {
            last = xValList.count() - 1;
            final = logist(par6);
            ui->doubleSpinBox_6_trans->setValue(final);
            t2min = exp(-1 * (pow((xValList.at(last) - peak2), 2)/dnsel));
        }
        for (i = 0; i < xValList.count(); i++)
        {
            xval = xValList.at(i);
            t1 = xval - peak;
            t2 = xval - peak2;
            t1term = exp(-t1 * t1 / upsel);
            t2term = exp(-t2 * t2 / dnsel);
            jn1 = logist(join1 * t1 / (1. + fabs(t1)));
            jn2 = logist(join2 * t2 / (1. + fabs(t2)));
            if (par5 > -999)
            {
                asc = init +
                        (apical_selex-init) * (t1term - t1min)
                        / (1.0 - t1min);
            }
            else
            {
                asc = apical_selex * t1term;
            }
            if (par6 > -999)
            {
                des = apical_selex +
                        (final-apical_selex) * (t2term - 1.0)
                        / (t2min - 1.0);
            }
            else
            {
                des = apical_selex * t2term;
            }
            join1Series->append(QPointF(xval, jn1));
            join2Series->append(QPointF(xval, jn2));
            ascendSeries->append(QPointF(xval, asc));
            dscendSeries->append(QPointF(xval, des));
            if (i < first)
                sel = 0.000001;
            else if (i <= last)
                sel = asc * (1.0-jn1) + jn1 * (apical_selex*(1.-jn2) + des * jn2);
            valSeries->append(QPointF(xval, sel));
        }
    }
}

// 20 #age selectivity: double normal with plateau
// case 23 size selectivity double_normal_plateau where final value can be greater than 1.0
// case 24 size selectivity using double_normal_plateau and lots of bells and whistles
// selex 20, 23, 24
void DialogSelexEquationView::dblNormEndpts()
{
    disconnect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    max1Changed(xValList.last()); min1Changed(xValList.first());
    max2Changed(3); min2Changed(-5);
    max3Changed(12); min3Changed(-5);
    max4Changed(10); min4Changed(-2);
    max5Changed(5); min5Changed(-15);
    max6Changed(5); min6Changed(-5);
    showSliders(6);
    ui->label_1_name->setText("Peak");
    ui->label_1_type->setText("Value");
    ui->label_2_name->setText("Top");
    ui->label_2_type->setText("Peak+Log");
    ui->label_3_name->setText("Asc-width");
    ui->label_3_type->setText("Exp");
    ui->label_4_name->setText("Dsc-width");
    ui->label_4_type->setText("Exp");
    ui->label_5_name->setText("Initial");
    ui->label_5_type->setText("Log");
    ui->label_6_name->setText("Final");
    ui->label_6_type->setText("Log");
    setParameterHeaders();
    if(equationNum == 23)
    {
        setTitle(QString("Pattern %1: Double Normal plateau").arg(
                     QString::number(equationNum)));
        ui->label_6_type->setText("Value");
        max6Changed(1); min6Changed(0);
    }
    else
    {
        setTitle(QString("Pattern %1: Double Normal with end-points").arg(
                     QString::number(equationNum)));
        ui->label_6_type->setText("Log");
    }
    ui->spinBox_steep_join1->setValue(20);
    ui->spinBox_steep_join2->setValue(20);

    connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    showBins(true);
    showJoins(2);

//    resetChart();

    join1Series = new QLineSeries(cht);
    join1Series->setPen(QPen(QBrush(Qt::darkYellow), 2));
    join1Series->setName(QString("Join1"));
    cht->addSeries(join1Series);

    join2Series = new QLineSeries(cht);
    join2Series->setPen(QPen(QBrush(Qt::yellow), 2));
    join2Series->setName(QString("Join2"));
    cht->addSeries(join2Series);

    ascendSeries = new QLineSeries(cht);
    ascendSeries->setPen(QPen(QBrush(Qt::green), 3));
    ascendSeries->setName(QString("Asc-Norm"));
    cht->addSeries(ascendSeries);

    dscendSeries = new QLineSeries(cht);
    dscendSeries->setPen(QPen(QBrush(Qt::blue), 3));
    dscendSeries->setName(QString("Desc-Norm"));
    cht->addSeries(dscendSeries);

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);

    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);

    ascendSeries->attachAxis(axisX);
    ascendSeries->attachAxis(axisY);
    dscendSeries->attachAxis(axisX);
    dscendSeries->attachAxis(axisY);
    join1Series->attachAxis(axisX);
    join1Series->attachAxis(axisY);
    join2Series->attachAxis(axisX);
    join2Series->attachAxis(axisY);

    cht->legend()->setVisible(true);
    cht->legend()->setAlignment(Qt::AlignLeft);

    if (parameters->rowCount() == 6)
    {
        if (equationNum == 23)
            updateDblNormPlateau();
        else
            updateDblNormEndpts();
    }
}

void DialogSelexEquationView::updateDblNormEndpts()
{
    float par1 = ui->doubleSpinBox_1_value->value();
    float par2 = ui->doubleSpinBox_2_value->value();
    float par3 = ui->doubleSpinBox_3_value->value();
    float par4 = ui->doubleSpinBox_4_value->value();
    float par5 = ui->doubleSpinBox_5_value->value();
    float par6 = ui->doubleSpinBox_6_value->value();
    if (par1 < min1) par1 = min1;
    if (par2 < min2) par2 = min2;
    if (par3 < min3) par3 = min3;
    if (par4 < min4) par4 = min4;
    if (par5 < min5) par5 = min5;
    if (par6 < min6) par6 = min6;

    float peak = par1;
    float top = peak+binWidth + (.99*binMax - peak - binWidth) * logist(par2);
    float asc_wd = exp(par3);
    float dsc_wd = exp(par4);
    float init = logist(par5);
    float final = logist(par6);
    float valmin, valmax, valminpow, valmaxpow;
    float limit, upsel, dnsel, jn1, jn2;
    int i;
    float xval = 0, sel = 0;

    ui->doubleSpinBox_1_trans->setValue(peak);
    ui->doubleSpinBox_2_trans->setValue(top);
    ui->doubleSpinBox_3_trans->setValue(asc_wd);
    ui->doubleSpinBox_4_trans->setValue(dsc_wd);
    ui->doubleSpinBox_5_trans->setValue(init);
    ui->doubleSpinBox_6_trans->setValue(final);

    firstPoints.clear();
    ascendSeries->clear();
    dscendSeries->clear();
    join1Series->clear();
    join2Series->clear();
    valSeries->clear();

    if (xValList.count() > 0)
    {
        limit = (par5 < -1000)? (-1000 - par5): -1;
        valmin = binMin + getBinStep() * getMidBin();
        valmax = binMax + getBinStep() * getMidBin();
        valminpow = pow((valmin - peak), 2);
        valmaxpow = pow((valmax - top), 2);
        valmin = exp(-1 * valminpow/asc_wd);
        valmax = exp(-1 * valmaxpow/dsc_wd);

        for (i = 0; i < xValList.count(); i++)
        {
            xval = xValList.at(i);
            upsel = exp(-1*(pow(xval - peak, 2))/asc_wd);
            if (par5 > -999)
                upsel = init + (1 - init) * (upsel - valmin)/(1-valmin);
            dnsel = exp(-1*(pow(xval - top, 2))/dsc_wd);
            if (par6 > -999)
                dnsel = (1 + (final - 1) * (dnsel - 1)/(valmax - 1));

            jn1 = logist(join1 *(xval-peak)/(1 + fabs(xval-peak)));
            jn2 = logist(join2 *(xval-top)/(1 + fabs(xval-top)));

            if (xval > limit)
                sel = upsel * (1-jn1) + jn1 * ((1-jn2) + (dnsel * jn2));
            else
                sel = 0.000001;

            ascendSeries->append(xval, upsel);
            dscendSeries->append(xval, dnsel);
            join1Series->append(xval, jn1);
            join2Series->append(xval, jn2);
            valSeries->append(xval, sel);
        }
    }
}

// case 25 size selectivity using exponential-logistic
// 26 #age selectivity: exponential logistic
/* peak = len_bins_m(1) + sp(2)*(len_bins_m(nlength)-len_bins_m(1));
   for (j=1;j<=nlength;j++)
   {
     sel(j) = mfexp(sp(3)*sp(1)*(peak-len_bins_m(j)))/
              (1.0-sp(3)*(1.0-mfexp(sp(1)*(peak-len_bins_m(j)))));
   }
*/
void DialogSelexEquationView::expLogistic()
{
    setTitle(QString("Pattern %1: Exponential-Logistic").arg(
                 QString::number(equationNum)));
    disconnect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    max1Changed(2); min1Changed(.02);
    max2Changed(.999); min2Changed(.001);
    max3Changed(.5); min3Changed(.001);
    connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    showSliders(3);
    ui->label_1_name->setText("Width");
    ui->label_1_type->setText("Value");
    ui->label_2_name->setText("Peak");
    ui->label_2_type->setText("Scaled");
    ui->label_3_name->setText("Tail Ht");
    ui->label_3_type->setText("Value");
    setParameterHeaders();
    showBins(true);
    showJoins(0);

//    resetChart();

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);
    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);

    if (parameters->rowCount() == 3)
        updateExpLogistic();
}

void DialogSelexEquationView::updateExpLogistic()
{
    int xRange = xValList.last() - xValList.first();
    float yVal = 0;
    float temp = 0;
    float xVal = 0;
    float par1 = ui->doubleSpinBox_1_value->value();
    float par2 = ui->doubleSpinBox_2_value->value();
    float par3 = ui->doubleSpinBox_3_value->value();
    if (par1 < min1) par1 = min1;
    if (par2 < min2) par2 = min2;
    if (par3 < min3) par3 = min3;

    float peak = xValList.first() + par2 * (xRange);
    ui->doubleSpinBox_1_trans->setValue(par1);
    ui->doubleSpinBox_2_trans->setValue(peak);
    ui->doubleSpinBox_3_trans->setValue(par3);

    valSeries->clear();

    for (int i = 0; i < xValList.count(); i++)
    {
        xVal = xValList.at(i);
        //sel_a(y,fs,1,a) = mfexp(sp(3)*sp(1)*(peak-r_ages(a)))/(1.0-sp(3)*(1.0-mfexp(sp(1)*(peak-r_ages(a)))));
        temp = 1.0 - par3 * (1 - exp(par1 * (peak - xVal)));
        yVal = exp(par3 * par1 * (peak - xVal))/temp;
        valSeries->append(QPointF(xVal, yVal));
    }
}

/** Returns the logistic of the requested value
 * 1/(1 + exp(-value)) */
float DialogSelexEquationView::logist(double value)
{
    float temp = exp(-value);
    return (1./(1. + temp));
}

// 14 #age selectivity - separate parm for each age
void DialogSelexEquationView::eachAge ()
{
    setTitle(QString("Pattern %1: Revise Age").arg(
                 QString::number(equationNum)));
/*    binMin = xValList.at(0);
    binMax = binMin + (binWidth * selex->getNumAges());
    ui->spinBox_bins_min->setValue(binMin);
    ui->spinBox_bins_max->setValue(binMax);*/

    showSliders(0);
    showBins(true);
    showJoins(0);
    setParameterHeaders();

//    resetChart();
    connect (parameters, SIGNAL(dataChanged()), this, SLOT(parametersChanged()));

    ascendSeries = new QLineSeries(cht);
    ascendSeries->setPen(QPen(QBrush(Qt::blue), 3));
    cht->addSeries(ascendSeries);

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);
    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);

    axisYalt->setTitleText("Value at age (blue)");
    cht->addAxis(axisYalt, Qt::AlignRight);
    ascendSeries->attachAxis(axisX);
    ascendSeries->attachAxis(axisYalt);

    if (parameters->rowCount() > xValList.count())
        updateEachAge();
}

void DialogSelexEquationView::updateEachAge ()
{
    float parm = 0.;
    float asc = 0.;
    float sel = 0.;
    float temp, max = 0, min = 0;
    float age = binMin + binMidWidth * binWidth;
    QString msg("");

    valSeries->clear();
    firstPoints.clear();
    if (numParams < xValList.count())
    {
        msg = QString("Fewer parameters than number of ages +1.");
        ui->label_title->setVisible(true);
    }
    else if (numParams > xValList.count())
    {
        msg = QString("More parameters than number of ages +1.");
        ui->label_title->setVisible(true);
    }
    ui->label_title->setText(msg);

    for (int i = 0; i < parameters->rowCount() || i < xValList.count(); i++)
    {
        parm = QString(parameters->getRowData(i).at(2)).toFloat();
        if (parm < -5) parm = -5.;
        if (parm > 9) parm = 9.;
        firstPoints.append(QPointF(age, parm));
        age += binWidth;
    }
    temp = 9. - maxYvalue(firstPoints);
    sel = 0.;

    for (int i = 0; i < firstPoints.count(); i++)
    {
        if (i < firstPoints.count() - 1)
        {
            parm = firstPoints.at(i + 1).y();
        }
        else
        {
            parm = firstPoints.at(i).y();
        }
        if (parm > -999)
        {
            asc = (parm + temp);
            sel = logist(asc);
        }
        ascendSeries->append(QPointF(firstPoints.at(i).x(), asc));
        valSeries->append(QPointF(firstPoints.at(i).x(), sel));
    }
    min = minYvalue(ascendSeries->points());
    max = maxYvalue(ascendSeries->points());
    axisYalt->setRange(min, max);
}

// 17 #age selectivity: each age has parameter as random walk
//    transformation is selex=exp(parm); some special codes
void DialogSelexEquationView::randomWalk (float scale)
{
    setTitle(QString("Pattern %1: Random Walk").arg(
                 QString::number(equationNum)));

    showSliders(0);
    showBins(true);
    showJoins(0);
    setParameterHeaders();

//    resetChart();
    connect (parameters, SIGNAL(dataChanged()), this, SLOT(parametersChanged()));

    ascendSeries = new QLineSeries(cht);
    ascendSeries->setPen(QPen(QBrush(Qt::blue), 3));
    cht->addSeries(ascendSeries);

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);
    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);
    axisY->setTitleText("Selex (red)");

    axisYalt->setTitleText("Use Parm (blue)");
    cht->addAxis(axisYalt, Qt::AlignRight);
    ascendSeries->attachAxis(axisX);
    ascendSeries->attachAxis(axisYalt);

    if (parameters->rowCount() > xValList.count())
        updateRandomWalk(scale);
}

void DialogSelexEquationView::updateRandomWalk (float scale)
{
    float useparm = 0.;
    float sel = 0.;
    float parm = 0.;
    float cumparm = 0;
    float temp = 0, mean = 0, max = 0;
    QList<float> parms;
//    int age = binMin;
    int lastage = binMax;
    int lastageIndex = xValList.count() - 1;
    int minAge = QString(parameters->getRowData(0).at(2)).toInt();
    int maxAge = QString(parameters->getRowData(1).at(2)).toInt();
    ascendSeries->clear();
    valSeries->clear();
    firstPoints.clear();

    if (special != 0)
        lastage = abs(special);
    for (int i = 0; i < xValList.count(); i++) {
        if (xValList.at(i) > lastage) {
            lastageIndex = i - 1;
            break;
        }
    }

    parm = QString(parameters->getRowData(0).at(2)).toFloat();
    if (parm == -1000)
    {
        parm = 0.0;
    }
    cumparm = useparm = parm;
    if (xValList.count() > 0)
    {
    valSeries->append(QPointF(xValList.at(0), parm));

    parms.append(parm);
    ascendSeries->append(QPointF(xValList.at(0), useparm));
    firstPoints.append(QPointF(xValList.at(0), cumparm));

    for (int i = 1; i <= lastageIndex; i++)
    {
        parm = QString(parameters->getRowData(i).at(2)).toFloat();
        parms.append(parm);
        if (parms[i] > -999)
            useparm = parms[i];
        if (fabs(useparm) > max)
            max = fabs(useparm);
        ascendSeries->append(QPointF(xValList.at(i), useparm));
        cumparm += useparm;
        firstPoints.append(QPointF(xValList.at(i), cumparm));
    }
    axisYalt->setRange(-max, max);

    if (scale == 0)
    {
        temp = maxYvalue(firstPoints);
    }
    else
    {
        int low_bin;
        int high_bin;
        float total = 0;
        if (minAge < 0)
        {
            minAge = 0;
        }
        if (maxAge > binMax)
        {
            maxAge = binMax;
        }
        if (maxAge < minAge)
            maxAge = minAge;
        if (minAge > maxAge)
            minAge = maxAge;

        low_bin = xValList.indexOf(minAge);
        high_bin = xValList.indexOf(maxAge);
//        sp(1) = low_bin;
//        sp(2) = high_bin;
//        temp = mean(tempvec_a(low_bin,high_bin));

        for (int i = low_bin; i <= high_bin; i++)
            total += firstPoints.at(i).y();
        temp = total / (high_bin - low_bin + 1);
    }

    for (int i = 1; i < firstPoints.count(); i++)
    {
        int next = i + 1 + scale;
        if ((next < parms.count()) && (parms.at(next) == -1000))
            sel = 0.0;
        else
            sel = exp(firstPoints.at(i).y() - temp);
        valSeries->append(QPointF(xValList.at(i), sel));
    }

    for (int i = firstPoints.count(); i < xValList.count(); i++)
    {
        if (special > 0)
            valSeries->append(QPointF(xValList.at(i), valSeries->points().at(i-1).y()));
        else
            valSeries->append(QPointF(xValList.at(i), 0.0));
    }
    }
}

// case 16 Coleraine - Gaussian age selectivity
void DialogSelexEquationView::coleGauss ()
{
    setTitle(QString("Pattern %1: Coleraine single Gaussian").arg(
                 QString::number(equationNum)));

    disconnect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    max1Changed(5); min1Changed(-5);
    max2Changed(40); min2Changed(-5);
    connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    showSliders(2);
    ui->label_1_name->setText("Age limit");
    ui->label_1_type->setText("Log");
    ui->label_2_name->setText("Decline scaling");
    ui->label_2_type->setText("Value");
    setParameterHeaders();
    showBins(true);
    showJoins(0);

//    resetChart();

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);
    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);

    if (parameters->rowCount() == 2)
        updateColeGauss();
}

// Coleraine - Gaussian
/*  t1 = 1/(1+mfexp(-sp(1)))*nages;
    for (a=0;a<=nages;a++)
    {
      if(a<t1)
      {
        sel_a(y,fs,1,a) = mfexp(-square(r_ages(a)-t1)/mfexp(sp(2)));
      }
      else
      {
        sel_a(y,fs,1,a)=1.0;
      }
    }
 */
void DialogSelexEquationView::updateColeGauss()
{
    int i;
    float temp = 0;
    float sel = 0;
    float par1 = ui->doubleSpinBox_1_value->value();
    float par2 = ui->doubleSpinBox_2_value->value();
    if (par1 < min1) par1 = min1;
    if (par2 < min2) par2 = min2;
    ui->doubleSpinBox_2_trans->setValue(par2);

    valSeries->clear();

    temp = static_cast<int>(logist(par1) * xValList.count());
    ui->doubleSpinBox_1_trans->setValue(xValList.at(temp));

    for (i = 0; i < temp; i++)
    {
        sel = exp(-1*pow(xValList.at(i) - temp, 2) / exp(par2));
        valSeries->append(QPointF(xValList.at(i), sel));
    }
    for (; i < xValList.count(); i++)
    {
        valSeries->append(QPointF(xValList.at(i), 1.0));
    }
}

// case 27 size selectivity using cubic spline
//  first N parameters are the spline knots; second N parameters are ln(selex) at the knot
//  scaled by max(raw vector) to achieve scale to 1.0
// case 42 size and selectivity using cubic spline
//  scaled by average of values at low bin through high bin
void DialogSelexEquationView::cubicSpline(float scale)
{
    int num = parameters->rowCount();

    if (scale > 0)
    {
        num -= 5;
        showSliders(5);
/*        max1Changed(xValList.count()); min1Changed(2);
        max2Changed(xValList.count()); min2Changed(1);
        max3Changed(2); min3Changed(0);
        max4Changed(1); min4Changed(-.001);
        max5Changed(.001); min5Changed(-1.0);*/
        ui->label_1_name->setText("ScaleLo");
        ui->label_1_type->setText("Value");
        ui->label_2_name->setText("ScaleHi");
        ui->label_2_type->setText("Value");
        ui->label_3_name->setText("Setup");
        ui->label_3_type->setText("Value");
        ui->label_4_name->setText("GradLo");
        ui->label_4_type->setText("Value");
        ui->label_5_name->setText("GradHi");
        ui->label_5_type->setText("Value");
    }
    else
    {
        num -= 3;
        showSliders(3);
/*        max1Changed(2); min1Changed(0);
        max2Changed(1); min2Changed(-.001);
        max3Changed(.001); min3Changed(-1.0);*/
        ui->label_1_name->setText("Setup");
        ui->label_1_type->setText("Value");
        ui->label_2_name->setText("GradLo");
        ui->label_2_type->setText("Value");
        ui->label_3_name->setText("GradHi");
        ui->label_3_type->setText("Value");
    }

    setTitle(QString("Pattern %1: Cubic Spline %2 nodes").arg(
                 QString::number(equationNum),
                 QString::number(num/2)));
    setParameterHeaders();
    showBins(true);
    showJoins(0);

 //   resetChart();
    connect (parameters, SIGNAL(dataChanged()), this, SLOT(parametersChanged()));

    ascendSeries = new QLineSeries(cht);
    ascendSeries->setPen(QPen(QBrush(Qt::blue), 3));
    cht->addSeries(ascendSeries);

    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);
    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);
    axisY->setTitleText("Selex (red)");

    ptSeries = new QScatterSeries(cht);
    ptSeries->setPen(QPen(QBrush(Qt::blue), 3));
    cht->addSeries(ptSeries);

    axisYalt->setTitleText("Cubic Nodes (blue)");
    cht->addAxis(axisYalt, Qt::AlignRight);
    ptSeries->attachAxis(axisX);
    ptSeries->attachAxis(axisYalt);
    ascendSeries->attachAxis(axisX);
    ascendSeries->attachAxis(axisYalt);

    if (num > 5 && (num % 2) == 0)
    {
        updateCubicSpline(scale);
    }
}

void DialogSelexEquationView::updateCubicSpline(float scale)
{
    float xval = 0, yval = 0;
    int start = 3 + scale;
    int num = parameters->rowCount() - start;
    int scaleLo = -1, scaleHi = -1;
    int setup = 0;
    float gradLo = 0;
    float gradHi = 0;
    float divisor = 1;
    int i;
    bool okay;
    QString msg;
    // QMap automatically sorts on key.
    QMap<float, float> pts;
    float maxVal, minVal, max, min;
    float altMaxVal, altMinVal, altMax, altMin;

    if ((num % 2) == 1 || num < 6)
    {
        msg = QString(tr("Incorrect number of parameters."));
        ui->label_title->setText(msg);
    }
    else
    {
        num /= 2;
        std::vector<double> X(num), Y(num);
        tk::spline cubicspl;

        if (scale < 1)
        {
            setup = ui->doubleSpinBox_1_value->value();
            gradLo = ui->doubleSpinBox_2_value->value();
            gradHi = ui->doubleSpinBox_3_value->value();
            ui->doubleSpinBox_1_trans->setValue(setup);
            ui->doubleSpinBox_2_trans->setValue(gradLo);
            ui->doubleSpinBox_3_trans->setValue(gradHi);
        }
        else
        {
            start = 5;
            scaleLo = ui->doubleSpinBox_1_value->value();
            scaleHi = ui->doubleSpinBox_2_value->value();
            setup = ui->doubleSpinBox_3_value->value();
            gradLo = ui->doubleSpinBox_4_value->value();
            gradHi = ui->doubleSpinBox_5_value->value();
            ui->doubleSpinBox_1_trans->setValue(scaleLo);
            ui->doubleSpinBox_2_trans->setValue(scaleHi);
            ui->doubleSpinBox_3_trans->setValue(setup);
            ui->doubleSpinBox_4_trans->setValue(gradLo);
            ui->doubleSpinBox_5_trans->setValue(gradHi);
            if (scaleLo > scaleHi)
            {
                xval = scaleHi;
                scaleHi = scaleLo;
                scaleLo = xval;
            }
        }

        switch (setup)
        {
        case 0:
            for (i = 0; i < num; i++)
                X[i] = (QString(parameters->getRowData(start++).at(2)).toFloat());
            break;
        case 1:
        case 2:
            int x0 = xValList.first();
            int xn = xValList.last();
            int num_1 = num - 1;
            float dist = xn - x0;
            float incr = dist / (num_1);
            X[0] = (x0 + (2.5 * dist) / 100);
            X[num_1] = (x0 + (97.5 * dist) / 100);
            for (i = 1; i < num_1; i++)
                X[i] = (X[i-1] + incr);
//            for (i = 0; i < num; i++)
//                parameters->setItem(start++, 2, new QStandardItem(QString::number(X[i])));
        }

        for (i = 0; i < num; i++)
        {
            Y[i] = (QString(parameters->getRowData(start++).at(2)).toFloat());
        }
        for (i = 0; i < num; i++)
            pts.insert(X[i], Y[i]);

        ptSeries->clear();
        QMapIterator<float, float> mi(pts);
        i = 0;
        while (mi.hasNext())
        {
            mi.next();
            X[i] = mi.key();
            Y[i] = mi.value();
            ptSeries->append(X[i], Y[i]);
            i++;
        }

        // currently it is required that X is already sorted
        cubicspl.set_boundary(tk::spline::first_deriv, gradLo,
                              tk::spline::first_deriv, gradHi);
        cubicspl.set_points(X, Y);

        firstPoints.clear();
        valSeries->clear();
        ascendSeries->clear();

        for (i = 0; i < xValList.count(); i++)
        {
            xval = xValList.at(i);
            yval = cubicspl(xval);
            firstPoints.append(QPointF(xval, yval));
        }
        ascendSeries->append(firstPoints);

        altMinVal = minYvalue (firstPoints);
        altMaxVal = maxYvalue (firstPoints);
//        range = maxVal - minVal;
//        if (range < 1.0)
//            minVal = maxVal - 1.0;
        altMax = altMinVal + ((altMaxVal - altMinVal) * 1.2);
        axisYalt->setRange(altMinVal, altMax);
        altMaxVal -= fabs(altMinVal);

        for (i = 0; i < firstPoints.count(); i++)
        {
            yval = firstPoints.at(i).y();
            firstPoints[i].setY(yval - altMinVal);
        }
        maxVal = maxYvalue(firstPoints);

        if (scale > 0)
        {
            divisor = fabs(aveYvalue(firstPoints, scaleLo, scaleHi));
        }
        else
        {
            divisor = fabs(maxVal);
        }
        if (divisor != 0.0)
        {
            for (i = 0; i < firstPoints.count(); i++)
                firstPoints[i].setY(firstPoints.at(i).y() / divisor);
        }
/*        for (i = 0; i < firstPoints.count(); i++)
        {
            yval = firstPoints[i].y();
            if (yval > 1.0)
                firstPoints[i].setY(1.0);
        }*/
        maxVal = maxYvalue (firstPoints);
        max = maxVal * 1.2;
        axisY->setRange(0.0, max);
        valSeries->append(firstPoints);
        if (scale > 1)
            axisYalt->setRange(altMinVal, (altMinVal + max));
    }
}

// 44 like age selex 17 but with separate parameters for males and with revised controls
void DialogSelexEquationView::twoSexRandomWalk()
{
    setTitle(QString("Pattern %1: Random Walk - female/male").arg(
                 QString::number(equationNum)));

    showSliders(genders + 2);
    showBins(true);
    showJoins(0);
    setParameterHeaders();

    resetChart();

    // female ln(selex)
    ascendSeries = new QLineSeries(cht);
    ascendSeries->setPen(QPen(QBrush(Qt::blue), 3));
    cht->addSeries(ascendSeries);

    // male ln(selex)
    dscendSeries = new QLineSeries(cht);
    dscendSeries->setPen(QPen(QBrush(Qt::green), 3));
    cht->addSeries(dscendSeries);

    // female selex
    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);
    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);

    // male selex
    join3Series = new QLineSeries(cht);
    join3Series->setPen(QPen(QBrush(Qt::magenta), 3));
    cht->addSeries(join3Series);
    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);

    axisY->setTitleText("Selex (red)");

    axisYalt->setTitleText("Use Parm (blue)");
    cht->addAxis(axisYalt, Qt::AlignRight);
    ascendSeries->attachAxis(axisX);
    ascendSeries->attachAxis(axisYalt);

    updateTwoSexRandom();
}

void DialogSelexEquationView::updateTwoSexRandom()
{
    float useparm = 0.;
    float sel = 0.;
    float parm = 0.;
    float cumparm = 0;
    float temp = 0, mean = 0, max = 0;
    QList<float> parms;
//    int age = binMin;
    int lastage = binMax;
    int lastageIndex = xValList.count() - 1;
    int minAge = QString(parameters->getRowData(0).at(2)).toInt();
    int maxAge = QString(parameters->getRowData(1).at(2)).toInt();
    ascendSeries->clear();
    dscendSeries->clear();
    valSeries->clear();
    join3Series->clear();
    firstPoints.clear();

    if (special != 0)
        lastage = abs(special);
    for (int i = 0; i < xValList.count(); i++) {
        if (xValList.at(i) > lastage) {
            lastageIndex = i - 1;
            break;
        }
    }

    parm = QString(parameters->getRowData(0).at(2)).toFloat();
    if (parm == -1000)
    {
        parm = 0.0;
    }
    cumparm = useparm = parm;
    if (xValList.count() > 0)
    {
    valSeries->append(QPointF(xValList.at(0), parm));

    parms.append(parm);
    ascendSeries->append(QPointF(xValList.at(0), useparm));
    firstPoints.append(QPointF(xValList.at(0), cumparm));

    for (int i = 1; i <= lastageIndex; i++)
    {
        parm = QString(parameters->getRowData(i).at(2)).toFloat();
        parms.append(parm);
        if (parms[i] > -999)
            useparm = parms[i];
        if (fabs(useparm) > max)
            max = fabs(useparm);
        ascendSeries->append(QPointF(xValList.at(i), useparm));
        cumparm += useparm;
        firstPoints.append(QPointF(xValList.at(i), cumparm));
    }
    axisYalt->setRange(-max, max);

    {
        int low_bin;
        int high_bin;
        float total = 0;
        if (minAge < 0)
        {
            minAge = 0;
        }
        if (maxAge > binMax)
        {
            maxAge = binMax;
        }
        if (maxAge < minAge)
            maxAge = minAge;
        if (minAge > maxAge)
            minAge = maxAge;

        low_bin = xValList.indexOf(minAge);
        high_bin = xValList.indexOf(maxAge);
//        sp(1) = low_bin;
//        sp(2) = high_bin;
//        temp = mean(tempvec_a(low_bin,high_bin));

        for (int i = low_bin; i <= high_bin; i++)
            total += firstPoints.at(i).y();
        temp = total / (high_bin - low_bin + 1);
    }

    for (int i = 1; i < firstPoints.count(); i++)
    {
        int next = i + 1;
        if ((next < parms.count()) && (parms.at(next) == -1000))
            sel = 0.0;
        else
            sel = exp(firstPoints.at(i).y() - temp);
        valSeries->append(QPointF(xValList.at(i), sel));
    }

    for (int i = firstPoints.count(); i < xValList.count(); i++)
    {
        if (special > 0)
            valSeries->append(QPointF(xValList.at(i), valSeries->points().at(i-1).y()));
        else
            valSeries->append(QPointF(xValList.at(i), 0.0));
    }
    }
}

// 45 similar to selex 14 but with separate parameters for males and with revised controls
void DialogSelexEquationView::twoSexEachAge()
{
    setTitle(QString("Pattern %1: Revise Age - female/male").arg(
                 QString::number(equationNum)));
    disconnect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    max1Changed(xValList.last());
    min1Changed(xValList.first());
    max2Changed(xValList.last());
    min2Changed(xValList.first());
    max3Changed(xValList.last());
    min3Changed(xValList.first());
    max4Changed(2.0);
    min4Changed(-1.0);
    connect (this, SIGNAL(numbersUpdated()), this, SLOT(update()));
    ui->label_1_name->setText("First age non-zero");
    ui->label_1_type->setText("Value");
    ui->label_2_name->setText("First age peak");
    ui->label_2_type->setText("Value");
    ui->label_3_name->setText("Last age peak");
    ui->label_3_type->setText("Value");
    ui->label_4_name->setText("Male ln(ratio)");
    ui->label_4_type->setText("Value");
    showSliders(genders + 2);
    showBins(true);
    showJoins(0);
    setParameterHeaders();

//    resetChart();
    connect (parameters, SIGNAL(dataChanged()), this, SLOT(parametersChanged()));

    // female ln(selex)
    ascendSeries = new QLineSeries(cht);
    ascendSeries->setPen(QPen(QBrush(Qt::blue), 3));
    cht->addSeries(ascendSeries);

    // female selex
    valSeries->setPen(QPen(QBrush(Qt::red), 3));
    cht->addSeries(valSeries);
    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);

    // male ln(selex)
    dscendSeries = new QLineSeries(cht);
    dscendSeries->setPen(QPen(QBrush(Qt::green), 3));
    cht->addSeries(dscendSeries);

    // male selex
    join3Series = new QLineSeries(cht);
    join3Series->setPen(QPen(QBrush(Qt::magenta), 3));
    cht->addSeries(join3Series);
    cht->addAxis(axisX, Qt::AlignBottom);
    cht->addAxis(axisY, Qt::AlignLeft);

    axisY->setTitleText("Selex (red)");

    axisYalt->setTitleText("Parm val (blue)");
    cht->addAxis(axisYalt, Qt::AlignRight);
    ascendSeries->attachAxis(axisX);
    ascendSeries->attachAxis(axisYalt);

    updateTwoSexEachAge();
}

void DialogSelexEquationView::updateTwoSexEachAge()
{
    float parmfm = 0.;
    float parmml = 0.;
    float temp = 0.;
    float sel = 0.;
    float max = 0, min = 0;
    float age = binMin + binMidWidth * binWidth;
    int firstage = 0, lastage = xValList.last();
    int i, j, minbin, maxbin;
    int offset = genders + 2;
    int maleoffset = offset + special;
    int endoffset = maleoffset + special;
    float par;
    float par1 = ui->doubleSpinBox_1_value->value();
    float par2 = ui->doubleSpinBox_2_value->value();
    float par3 = ui->doubleSpinBox_3_value->value();
    float mlratio = 0;
    if (genders > 1)
        mlratio = ui->doubleSpinBox_4_value->value();
    QString msg("");

    firstPoints.clear();
    ascendSeries->clear();
    valSeries->clear();
    dscendSeries->clear();
    join3Series->clear();

    firstage = par1;
    if (par2 > par3)
    {
        minbin = par3;
        maxbin = par2;
    }
    else
    {
        minbin = par2;
        maxbin = par3;
    }
    if (firstage > 0)
    {
        ascendSeries->append(QPointF(xValList.first(), 0.0));
    }
    if (firstage > 1)
    {
        ascendSeries->append(QPointF(xValList.at(firstage - 1), 0.0));
    }
    for (i = firstage, j = offset; j < maleoffset; i++, j++)
    {
        par = QString(parameters->getRowData(j).at(2)).toFloat();
        if (par != -999)
            parmfm = par;
        ascendSeries->append(QPointF(xValList.at(i), parmfm));
    }
    for (; i < xValList.count(); i++)
    {
        ascendSeries->append(QPointF(xValList.at(i), parmfm));
    }
    min = minYvalue(ascendSeries->points());
    max = maxYvalue(ascendSeries->points());
    axisYalt->setRange(min, max);

    // female selex
    firstPoints.clear();
    fillValues(ascendSeries->points(), xValList, firstPoints);
//    ascendSeries->append(firstPoints);

    temp = 8. - aveYvalue(firstPoints, minbin, maxbin);
    sel = 0.;

    for (int i = 0; i < firstPoints.count(); i++)
    {
        sel = logist(firstPoints.at(i).y() + temp);
        valSeries->append(QPointF(firstPoints.at(i).x(), sel));
    }
    if (genders > 1)
    {
        if (firstage > 0)
        {
            dscendSeries->append(QPointF(xValList.first(), 0.0));
        }
        if (firstage > 1)
        {
            dscendSeries->append(QPointF(xValList.at(firstage - 1), 0.0));
        }
        for (i = firstage, j = offset; j < maleoffset; i++, j++)
        {
            par = QString(parameters->getRowData(j + special).at(2)).toFloat();
            if (par == -1000)
                parmml = QPointF(ascendSeries->points().at(i)).x();
            else if (par != -999)
                parmml = par;
            dscendSeries->append(QPointF(xValList.at(i), parmml));
        }
        for (; i < xValList.count(); i++)
        {
            dscendSeries->append(QPointF(xValList.at(i), parmml));
        }

        // male selex
        firstPoints.clear();
        fillValues(dscendSeries->points(), xValList, firstPoints);
        valSeries->append(firstPoints);

        temp = 8. - aveYvalue(firstPoints, minbin, maxbin);
        sel = 0.;

        for (int i = 0; i < firstPoints.count(); i++)
        {
            sel = logist(firstPoints.at(i).y() + temp);
            join3Series->append(QPointF(firstPoints.at(i).x(), sel));
        }
    }
}



