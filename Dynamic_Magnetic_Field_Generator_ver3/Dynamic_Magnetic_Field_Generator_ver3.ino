// 20/12/07 ver 2.10 Kirihara
// 20/12/16 ver 2.20 Kirihara
// 20/12/23 ver 2.30 Kirihara
// 21/03/02 ver 2.35 Kirihara PID���� CalcImput()��Kp Ki Kd ��20���v���[�g�p�ɏ��������܂����D���S�������܂��D
//                            Ki�������Ă���̂Ŏ~�܂��Ă����΂炭����Γd���オ���ē����Ǝv���܂����C�z���Ȃ��ꍇ�A�����������D
//                            �d���̕ω��̃I�[�o�[�V���[�g���傫���ꍇ��Kd�����炵�Ă��������D�r���Ŏ~�܂�ꍇ��Ki���グ�Ă��������D
//                            10���v���[�g�Ȃ�Kp=0.02f�@Ki=0.1f Kd=0.02f �ʂ����傤�ǂ����ł��D
// 21/10/** ver 3.00 Kirihara ���ԕω��֐���ύX
#include "DualTB9051FTGMotorShield.h"

DualTB9051FTGMotorShield md;
const int LED = 4;
const float RPM_switch[4] = { 0.0f,30.0f,60.0f,240.0f }; //20���̏ꍇ 30rpm��5Hz 60rpm��10Hz 240rpm��40Hz
const float timer_limit = 86400.0f; //86400�b=1���Œ�~�D���ݓ��͂́uImputV*ImputMulti()+ImputAdd()�v�@ImputMulti�̒��g��1-0.5*H((���ݎ���)-timer_limit), ImputAdd�̒��g��0�ɂ��Ă���܂��D
const float GeerRatio = 20.4f;

float timer_start2, timer_delta2;

//===============���ԊǗ��p�ϐ� ProgramedRPM(millis())=================
const int mode = 1; //0�ŃX�C�b�`�R���g���[�����@1�ł��炩���߃v���O�����������ԕω��Ői��
const float time_schedule[4] = { 1/60,2/60,0.5/60,0.5/60 }; //��]����ύX�����鎞��(�J�n����̗݌v���Ԃł͂Ȃ��C�e��Ԃ̒���:h)
const float RPM_schedule[4] = { 30,-1,60,240 }; //��]��, time_schedule�ƑΉ��C-1.0�Ő��`�ω�
int length=(int)sizeof(RPM_schedule) / sizeof(float);
float time_length;

//===============�X�C�b�`�ǂݎ��ϐ� ReadSwitch()======================
//�X�C�b�`�ǂݎ��s��;
float targ_rpm, targ_rpm_bf = 0; //�ϐ�������;
int n = 0; //������;
const int analogPin = 3; //3�ԃs���œǂݎ��;

//==============���[�^�[�p���X�ǂݎ�� Countencorder()==================
//Green=encGND Blue=encVcc

//Yellow=encOUT1
const int outputA = 3;
//White=encOUT2
const int outputB = 5;
//�G���R�[�_�[�M��
int aState, aLastState;

//================���[�^�[�p���X�ǂݎ�� CalcRPM()=======================
//���]�p���X��
const float OneRotate = 12.0f;
//���Ԍv���p�֐�
float timer_start1, timer_delta1;
//�G���R�[�_�[�J�E���g
int counter = 0;

//================PID���� CalcImput()=================================
//���͐M��
float ImputV, ImputV_bf = 0;

//PID�̕ϐ��C�l���傫���قǕω��ʂ��傫���Ȃ�
const float Kp = 0.006f;
const float Ki = 0.03f;
const float Kd = 0.006f;
float ImputVmax = 400;
float DeltaRpmBf = 0, DeltaRpmAf = 0;
float Integral;


//��]�L�^�p
float rpm = 0, TempRpm = 0;

//���[�^���[�X�C�b�`��ǂݍ���ŖڕWrpm��Ԃ�;
void setup()
{
    Serial.begin(9600);
    Serial.print("\n");
    if (mode == 1) {
        if ((int)sizeof(RPM_schedule) / sizeof(float) != (int)sizeof(time_schedule) / sizeof(float)) {
            Serial.println("�ݒ�̎��ԂƉ�]���̒�������v���܂���");
        }
        Serial.print(Display_schedule());

        for (int i = 0;i < length;i++) {
            time_length += time_schedule[i];
        }
    }
    
    //Serial.println("test finish");

    //���[�^�[�h���C�o����
    md.init();
    md.enableDrivers();
    delay(1);

    //�G���R�[�_�[����
    pinMode(outputA, INPUT);
    pinMode(outputB, INPUT);
    aLastState = digitalRead(outputA);
    timer_start1 = millis();
    timer_start2 = millis();

    //�X�C�b�`����
    targ_rpm = 0;
}

void loop()
{
    //md.enableDrivers();
    //delay(1); // wait for drivers to be enabled so fault pins are no longer low
    md.setM1Speed(ImputV);
    Countencorder();
    timer_delta1 = millis() - timer_start1;
    timer_delta2 = millis() - timer_start2;
    //0.3�b���ƂɃ��[�^�[�X�V
    if (timer_delta1 > 300) {
        rpm = CalcRPM();
        if (mode == 0) {
            targ_rpm = ReadSwitch() * ImputMulti(millis()) + ImputAdd(millis() / 1000.0f);
        }
        else if (mode == 1) {
            targ_rpm = Programed_rpm(millis());
        }
        timer_start1 = millis();
    }
    if (timer_delta2 > 1000) {
        //�f�[�^�\�� ����[s], �ڕW��]��[rpm],���݂̉�]��[rpm],���͓d��[V]�ō\���@1s���ƂɍX�V
        Serial.print(millis() / 1000.0f);
        Serial.print(",");
        //Serial.print("");
        Serial.print(targ_rpm);
        Serial.print(",");
        //Serial.print(": ");
        Serial.print(rpm);
        Serial.print(",");
        ImputV = CalcPID();
        //Serial.print("ImputV:");
        if (targ_rpm == 0 && rpm == 0) {
            ImputV = 0;
            if (time_length < millis() / (3600000.0f) ){
                Serial.println("");
                Serial.print("Programed schedule finished");
                    while (true)
                    {
                        //�I���㖳�����[�v
                    }
            }
        }
        Serial.println(ImputV / 400 * 12);
        timer_start2 = millis();
    }
}

float ReadSwitch() {
    // put your main code here, to run repeatedly:
    float max_val = 1024.0f;
    float val = analogRead(analogPin);
    //Serial.print( " : " );
    if (val < max_val * 0.25f) {
        if (targ_rpm != RPM_switch[0]) {
            //Serial.print("Off\n");
        }
        return RPM_switch[0];
    }
    else if (max_val * 0.25f <= val && val < max_val * 0.5f) {
        if (targ_rpm != RPM_switch[1]) {
            //Serial.print("Low\n");
        }
        return RPM_switch[1];
    }
    else if (max_val * 0.5f <= val && val < max_val * 0.75f) {
        if (targ_rpm != RPM_switch[2]) {
            //Serial.print("Middle\n");
        }
        return RPM_switch[2];
    }
    else {
        if (targ_rpm != RPM_switch[3]) {
            //Serial.print("High\n");
        }
        return RPM_switch[3];
    }
}

void Countencorder() {
    aState = digitalRead(outputA); // Reads the "current" state of the outputA
    // If the previous and the current state of the outputA are different, that means a Pulse has occured
    if (aState != aLastState && aState != 0) {
        // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
        if (digitalRead(outputB) != aState) {
        }
        else {
            counter++;
        }
        //Serial.print("Position: ");
        //Serial.println(counter);
    }
    aLastState = aState; // Updates the previous state of the outputA with the current state
}

void stopIfFault() {
    if (md.getM1Fault()) {
        Serial.println("M1 fault");
        while (1);
    }
    if (md.getM2Fault()) {
        Serial.println("M2 fault");
        while (1);
    }
}

float CalcRPM() {
    timer_delta1 = millis() - timer_start1;
    float TempRpm = 60.0f / timer_delta1 * 1000.0f * counter / (OneRotate * GeerRatio);
    //�v���l�̏�����
    counter = 0;
    return TempRpm;
}

float CalcPID() {
    float p, i, d;
    DeltaRpmBf = DeltaRpmAf;
    DeltaRpmAf = targ_rpm - rpm;

    p = Kp * DeltaRpmAf;
    d = Kd * (DeltaRpmAf - DeltaRpmBf) / timer_delta1 * 1000;
    Integral += (DeltaRpmAf + DeltaRpmBf) / 2 * timer_delta1 / 1000;

    i = Ki * Integral;

    if (p + i + d > 0) {
        if (p + i + d < ImputVmax) {
            return p + i + d;
        }
        else {
            return ImputVmax;
        }
    }
    else {
        return 0;
    }
}

float ImputMulti(float t) {
    if (t < timer_limit) {
        return 1;
    }
    else {
        return 0;
    }
}

float ImputAdd(float t) {
    return 0;
}

float Programed_rpm(float t) {
    float time = t / (3600000); //�o�ߎ���[h]�̓��o 60sec*60min*1000ms��[ms]��[h]�ɕϊ�
    float keika = 0;
    int phase = length;
    for (size_t i = 0; i < length; i++)
    {
        keika = keika + time_schedule[i];
        if (keika - time > 0) {
            phase = i;
            break;
        }
    }
    if (phase == 0) {
        if ((int)RPM_schedule[phase]==-1) {
            return RPM_schedule[0] / time_schedule[0] * time;
        }
        else
        {
            return RPM_schedule[phase];
        }
    }
    else if (phase == length-1) {
        if ((int)RPM_schedule[phase] == -1) {
            return -RPM_schedule[phase - 1] / time_schedule[phase] * (time-keika+time_schedule[phase]) + RPM_schedule[phase - 1];
        }
        else {
            return RPM_schedule[phase];
        }
    }
    else if (phase == length) {
        return 0.0f;
    }
    else {
        if ((int)RPM_schedule[phase] == -1) {
            return (RPM_schedule[phase + 1] - RPM_schedule[phase - 1]) / time_schedule[phase] * (time - keika + time_schedule[phase]) + RPM_schedule[phase - 1];
        }
        else {
            return RPM_schedule[phase];
        }
    }
}

String Display_schedule() {
    String message = "Time Schedule\n";
    for (int i = 0; i < sizeof(RPM_schedule) / sizeof(float); i++) {
        if (RPM_schedule[i] < 0 && RPM_schedule[i] > -1.5) {
            if (i == 0) {
                message += "0rpm~" + String(RPM_schedule[i]) + "rpm(" + String(time_schedule[i]) + "h)->";
            }
            else if (i == sizeof(RPM_schedule) / sizeof(float) - 1) {
                message += String(RPM_schedule[i]) + "rpm~0rpm" + "(" + String(time_schedule[i]) + "h)\n";
            }
            else {
                message += String(RPM_schedule[i - 1]) + "rpm~" + String(RPM_schedule[i + 1]) + "rpm" + "(" + String(time_schedule[i]) + "h)->";
            }
        }
        else if (i == sizeof(RPM_schedule) / sizeof(float) - 1) {
            message += String(RPM_schedule[i]) + "rpm" + "(" + String(time_schedule[i]) + "h)\n";
        }
        else {
            message += String(RPM_schedule[i]) + "rpm" + "(" + String(time_schedule[i]) + "h)->";
        }
    }
    return message;
}