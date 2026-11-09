# Калибровка центра шарика контактного щупа на роботе по эталонной сфере: модели для ММК

Документ содержит математику, которая, вероятно, понадобится для математической модели калибровки (ММК, шаг 5 порядка исследования — теория, раздел 1): модель регистрации касания, оценивание параметров методом наименьших квадратов, ошибки робота и ошибки найденных параметров, задержку сигнала, учёт различий хода до срабатывания. Как ММК эта математика не принята: ММК определяется после отбора факторов влияния (шаг 4). Выводы документа верны при допущениях, оговорённых в них.

Документ `docs/calibration/ROBOT_PROBE_SPHERE_CALIBRATION_THEORY_RU.md` далее называется «теория», `docs/calibration/ROBOT_PROBE_SPHERE_CALIBRATION_LITERATURE_RU.md` — «анализ литературы». Номера формул этого документа начинаются с буквы m, например (m.2.2.19); номера формул без буквы — формулы теории. Разделы теории и анализа литературы названы с именем документа; остальные номера разделов — разделы этого документа. Термины и обозначения — теория, раздел 2.

## 1. Модель регистрации касания

Модель описывает только поиск: от точки ожидания ($t = 0$) до обнаружения касания. Предподход и торможение после обнаружения в модель не входят.

Движение TCP при поиске:

$$
p_a(t) = p_a(0) + v_s\,t
$$

где $p_a(t)$ — координата TCP вдоль направления подхода в момент $t$;
&emsp;&emsp;$p_a(0)$ — координата точки ожидания;
&emsp;&emsp;$v_s$ — скорость поиска;
&emsp;&emsp;$t$ — время от начала поиска.

Момент касания:

$$
t_c = \frac{p_a(t_c) - p_a(0)}{v_s}
$$

где $t_c$ — момент касания;
&emsp;&emsp;$p_a(t_c)$ — координата TCP в момент касания;
&emsp;&emsp;$p_a(0)$ — координата точки ожидания;
&emsp;&emsp;$v_s$ — скорость поиска.

До касания стилус не отклонён. После касания реальный шарик стоит на поверхности, а TCP продолжает движение, поэтому отклонение стилуса равно пути TCP после касания:

$$
\delta(t) = 0 \ \text{при } t < t_c, \qquad \delta(t) = p_a(t) - p_a(t_c) = v_s\,(t - t_c) \ \text{при } t \ge t_c
$$

где $\delta(t)$ — отклонение стилуса в момент $t$;
&emsp;&emsp;$p_a(t)$ — координата TCP в момент $t$;
&emsp;&emsp;$p_a(t_c)$ — координата TCP в момент касания;
&emsp;&emsp;$v_s$ — скорость поиска;
&emsp;&emsp;$t_c$ — момент касания.

Щуп срабатывает, когда отклонение стилуса достигает хода до срабатывания:

$$
t_{trig} = t_c + \frac{\delta_p}{v_s}
$$

где $t_{trig}$ — момент срабатывания щупа;
&emsp;&emsp;$t_c$ — момент касания;
&emsp;&emsp;$\delta_p$ — ход до срабатывания;
&emsp;&emsp;$v_s$ — скорость поиска.

Контроллер обнаруживает касание через задержку сигнала после срабатывания:

$$
t_{det}(v_s) = t_{trig} + t_{lat} = t_c + \frac{\delta_p}{v_s} + t_{lat}
$$

где $t_{det}(v_s)$ — момент обнаружения касания;
&emsp;&emsp;$t_{trig}$ — момент срабатывания щупа;
&emsp;&emsp;$t_{lat}$ — задержка сигнала;
&emsp;&emsp;$t_c$ — момент касания;
&emsp;&emsp;$\delta_p$ — ход до срабатывания;
&emsp;&emsp;$v_s$ — скорость поиска.

Зарегистрированная координата:

$$
p_{a.det}(v_s) = p_a(t_{det}(v_s)) = p_a(t_c) + \delta_p + v_s\,t_{lat}
$$

$$
\boxed{p_{a.det}(v_s) = p_a(t_c) + \delta_p + v_s\,t_{lat}}
\tag{m.1.1}
$$

где $p_{a.det}(v_s)$ — зарегистрированная координата при скорости поиска $v_s$;
&emsp;&emsp;$p_a(t_{det}(v_s))$ — координата TCP в момент обнаружения касания;
&emsp;&emsp;$p_a(t_c)$ — координата TCP в момент касания;
&emsp;&emsp;$\delta_p$ — ход до срабатывания;
&emsp;&emsp;$v_s$ — скорость поиска;
&emsp;&emsp;$t_{lat}$ — задержка сигнала.

Ход до срабатывания и перебег шарика $v_s\,t_{lat}$ проходятся последовательно, а не одновременно: $\delta_p$ — путь TCP от касания до срабатывания, $v_s\,t_{lat}$ — путь TCP от срабатывания до обнаружения. Стилус всё это время изгибается, и в момент обнаружения его отклонение равно их сумме.

Допущения модели:

- Скорость TCP постоянна и равна $v_s$ от касания до обнаружения, поэтому касание должно произойти на поиске, а не на предподходе.
- Всё, кроме стилуса, жёсткое: сфера, её держатель, крепление щупа и робот при малой силе срабатывания.
- Задержка сигнала меняется от касания к касанию случайно (раздел 5).

## 2. Оценивание параметров калибровки методом наименьших квадратов (МНК)

Модель калибровки рассматривается в трёх случаях; каждый следующий случай добавляет одно явление из раздела 1.

| <nobr>Случай</nobr>              | <nobr>Что</nobr> <nobr>добавляется</nobr>                                                                              | <nobr>Расстояние</nobr> <nobr>от</nobr> <nobr>центра</nobr> <nobr>сферы</nobr> <nobr>до</nobr> <nobr>зарегистрированного</nobr> <nobr>центра</nobr> <nobr>шарика</nobr>       |
| ------------------- | -------------------------------------------------------------------------------------------- | --------------------------------------------------------------------- |
| 1 (разделы 2.1–2.3) | ничего: шарик регистрируется в момент касания, отличие касаний друг от друга — только шум    | известно и одинаково у всех касаний: $L$                              |
| 2 (раздел 2.4)      | постоянный перебег шарика после касания: одна и та же величина при всех точках и ориентациях | неизвестно и одинаково у всех касаний: $\rho$ — один искомый параметр |
| 3 (раздел 2.5)      | ход до срабатывания зависит от направления нормальной силы в точке касания                                            | неизвестно и своё для каждой пары $(\tau, \psi)$: $\rho_i$            |

### 2.1. Случай 1: регистрация без перебега шарика

Допущение случая: в момент обнаружения касания центр шарика находится на расстоянии $L = r_b + r_s$ от центра сферы; отличие отдельного касания от этого значения — только шум (раздел 2.3).

Центр шарика в {B} при касании $i$ вычисляется по углам сочленений:

$$
{}^{B}\bar{p}_r(\bar{q}_i) = {}^{B}\bar{p}_F(\bar{q}_i) + {}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{p}_r
$$

где $\bar{q}_i$ — углы сочленений A1…A6 в момент обнаружения касания $i$, градусы;
&emsp;&emsp;${}^{B}\bar{p}_r(\bar{q}_i)$ — рассчитанный центр шарика в {B} при касании $i$, мм;
&emsp;&emsp;${}^{B}\bar{p}_F(\bar{q}_i),\ {}^{B}R_F(\bar{q}_i)$ — положение начала {F} в {B} и поворот {F} относительно {B};
&emsp;&emsp;${}^{F}\bar{p}_r$ — центр шарика в {F}, мм; не зависит от $\bar{q}$;
&emsp;&emsp;${}^{B}\bar{p}_s$ — центр сферы в {B}, мм.

Уравнение касания $i$:

$$
\left\| {}^{B}\bar{p}_r(\bar{q}_i) - {}^{B}\bar{p}_s \right\| = L, \qquad i = 1 \ldots m
$$

где $m$ — число касаний одной калибровки.

| <nobr>Величина</nobr>                                                                     | <nobr>Статус</nobr>                                        | <nobr>Откуда</nobr>                                                                                                           |
| ---------------------------------------------------------------------------- | --------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| углы сочленений $\bar{q}_i$, 6 чисел на касание                              | измеряются                                    | система управления робота в момент обнаружения касания                                                           |
| положение и поворот фланца в {B} при касании $i$                             | вычисляются по $\bar{q}_i$                    | номинальная прямая задача кинематики; ошибки модели робота считаются нулевыми — допущение; его снятие — раздел 3 |
| $L$                                                                          | известно                                      | номинальные радиусы шарика и сферы                                                                               |
| центр шарика в {F}, 3 числа                                                  | искомый параметр; результат калибровки        | начальное приближение — по чертежу или измерению щупа вне робота                                                 |
| центр сферы в {B}, 3 числа                                                   | искомый параметр; нужен только для оценивания | начальное приближение — по грубым предварительным касаниям                                                       |
| широта и долгота точки касания, наклон щупа, поворот щупа вокруг оси стилуса | известны; в уравнение касания не входят       | план касаний; задают только то, куда робот ведёт шарик                                                           |

Вектор искомых параметров случая 1 — три составляющие центра шарика в {F} и три составляющие центра сферы в {B}, мм:

$$
\bar{x} = \begin{bmatrix} {}^{F}p_{r,x} & {}^{F}p_{r,y} & {}^{F}p_{r,z} & {}^{B}p_{s,x} & {}^{B}p_{s,y} & {}^{B}p_{s,z} \end{bmatrix}^{T}
\tag{m.2.1.1}
$$

Число искомых параметров случая 1: $n = 6$.

Невязка уравнения касания $i$ — на сколько миллиметров рассчитанный центр шарика не достаёт до оболочки радиуса $L$ вокруг центра сферы (минус) или выходит за неё (плюс):

$$
\varepsilon_i(\bar{x}) = \left\| {}^{B}\bar{p}_F(\bar{q}_i) + {}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{p}_r - {}^{B}\bar{p}_s \right\| - L
\tag{m.2.1.2}
$$

Оценивание по МНК находит вектор параметров с наименьшей суммой квадратов невязок:

$$
\Phi(\bar{x}) = \sum_{i=1}^{m} \varepsilon_i(\bar{x})^2 = \bar{\varepsilon}(\bar{x})^T\,\bar{\varepsilon}(\bar{x}),
\qquad
\hat{\bar{x}} = \arg\min_{\bar{x} \in \mathbb{R}^n} \Phi(\bar{x})
\tag{m.2.1.3}
$$

где $\Phi:\ \mathbb{R}^n \to \mathbb{R}$ — целевая функция МНК, сумма квадратов невязок, мм²;
&emsp;&emsp;$\bar{\varepsilon}(\bar{x}) = \left(\varepsilon_1(\bar{x}), \ldots, \varepsilon_m(\bar{x})\right)^T$ — вектор-столбец невязок размером $m \times 1$, мм;
&emsp;&emsp;$\hat{\bar{x}}$ — вектор параметров, найденный МНК.

Невязки после оценивания — значения $\varepsilon_i(\hat{\bar{x}})$. Пример с условными значениями: при найденном $\bar{x}$ рассчитанный центр шарика касания 5 отстоит от найденного центра сферы на 13,990 мм; невязка $\varepsilon_5 = 13{,}990 - 14 = -0{,}010$ мм.

### 2.2. Идентифицируемость параметров случая 1

Параметры идентифицируемы, если наименьшую сумму квадратов невязок даёт только один вектор параметров. Если два разных вектора дают одинаковые невязки при всех касаниях, оценивание по МНК не может выбрать между ними: такие параметры неидентифицируемы. Проверяется это по матрице Якоби невязок: она входит в уравнение (m.2.2.2) — необходимое условие минимума.

#### 2.2.1. Условие минимума суммы квадратов невязок

Получим необходимое условие минимума суммы квадратов невязок (m.2.1.3), записанное через матрицу Якоби невязок.

Вектор $\hat{\bar{x}}$ из (m.2.1.3) — точка минимума $\Phi(\bar{x})$. В точке минимума частные производные $\Phi$ по всем параметрам равны нулю. Частная производная по параметру $j$:

$$
\frac{\partial \Phi(\bar{x})}{\partial x_j} = \frac{\partial}{\partial x_j}\sum_{i=1}^{m} \varepsilon_i(\bar{x})^2 = \sum_{i=1}^{m} \frac{\partial}{\partial x_j}\varepsilon_i(\bar{x})^2 = \sum_{i=1}^{m} 2\,\varepsilon_i(\bar{x})\,\frac{\partial \varepsilon_i(\bar{x})}{\partial x_j},
\qquad j = 1 \ldots n
$$

где $x_j$ — элемент $j$ вектора $\bar{x}$.

Частные производные невязок по параметрам образуют матрицу Якоби невязок:

$$
J_{ij}(\bar{x}) = \frac{\partial \varepsilon_i(\bar{x})}{\partial x_j}, \qquad i = 1 \ldots m, \quad j = 1 \ldots n
\tag{m.2.2.1}
$$

где $J(\bar{x})$ — матрица размером $m \times n$.

Подставим (m.2.2.1) в производную $\Phi$:

$$
\frac{\partial \Phi(\bar{x})}{\partial x_j} = 2\sum_{i=1}^{m} J_{ij}(\bar{x})\,\varepsilon_i(\bar{x}) = 2\left(J(\bar{x})^T\,\bar{\varepsilon}(\bar{x})\right)_j
$$

Поэтому необходимое условие минимума для всех $j = 1 \ldots n$ сразу:

$$
\boxed{J(\hat{\bar{x}})^T\,\bar{\varepsilon}(\hat{\bar{x}}) = \bar{0}}
\tag{m.2.2.2}
$$

где $\bar{0}$ — нулевой вектор-столбец размером $n \times 1$.

Система (m.2.2.2) — нормальные уравнения нелинейного МНК. Строка $J$ выводится в 6.2.2, явный вид системы для калибровки — в 6.2.3, её решение методом Гаусса — Ньютона и единственность решения — в 6.2.4, условия на набор касаний — в 6.2.5.

#### 2.2.2. Матрица Якоби невязок

Получим строку $i$ матрицы Якоби (m.2.2.1) в явном виде: через орты ${}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})$, ${}^{F}\bar{e}_{sr}(\bar{q}_i,\bar{x})$ и вектор ${}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})$ при касании $i$.

Вектор искомых параметров (m.2.1.1) состоит из двух блоков: элементы 1–3 — координаты центра шарика в {F}, элементы 4–6 — координаты центра сферы в {B}:

$$
\bar{x} = \begin{bmatrix} {}^{F}\bar{p}_r \\ {}^{B}\bar{p}_s \end{bmatrix}
$$

Строка $i$ матрицы Якоби (m.2.2.1) в блочной записи:

$$
\begin{bmatrix} J_{i1} & J_{i2} & J_{i3} \end{bmatrix} = \frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{F}\bar{p}_r},
\qquad
\begin{bmatrix} J_{i4} & J_{i5} & J_{i6} \end{bmatrix} = \frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{B}\bar{p}_s}
$$

где $\partial \varepsilon_i(\bar{x}) / \partial\,{}^{F}\bar{p}_r$, $\partial \varepsilon_i(\bar{x}) / \partial\,{}^{B}\bar{p}_s$ — строки частных производных $\varepsilon_i(\bar{x})$ по элементам ${}^{F}\bar{p}_r$ и ${}^{B}\bar{p}_s$.

Вектор, проведённый из центра сферы (точка $s$) в рассчитанный центр шарика (точка $r$) при касании $i$, выраженный в {B}, его орт и невязка (m.2.1.2) через этот вектор:

$$
\begin{gathered}
{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x}) = {}^{B}\bar{p}_F(\bar{q}_i) + {}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{p}_r - {}^{B}\bar{p}_s,
\qquad
{}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x}) = \frac{{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})}{\left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\|},
\qquad
\varepsilon_i(\bar{x}) = \left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\| - L
\end{gathered}
\tag{m.2.2.3}
$$

Длина вектора — расстояние между центрами, около $L$.

Ниже строка $i$ выводится в четыре шага. Углы сочленений $\bar{q}_i$ измерены и от $\bar{x}$ не зависят, поэтому положение и поворот фланца при касании $i$ при дифференцировании по $\bar{x}$ постоянны.

Шаг 1 — правило дифференцирования сложной функции. $L$ постоянна, и невязка зависит от параметров только через вектор:

$$
\frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{F}\bar{p}_r} = \frac{\partial \left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\|}{\partial\,{}^{B}\bar{r}_{sr}}\,\frac{\partial\,{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})}{\partial\,{}^{F}\bar{p}_r},
\qquad
\frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{B}\bar{p}_s} = \frac{\partial \left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\|}{\partial\,{}^{B}\bar{r}_{sr}}\,\frac{\partial\,{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})}{\partial\,{}^{B}\bar{p}_s}
\tag{m.2.2.4}
$$

Шаг 2 — производная длины вектора по его координатам. Координаты вектора в {B} обозначаются той же буквой $r$ с индексами $sr$ и номером координаты $k \in \{x, y, z\}$; длина — корень из суммы квадратов координат:

$$
{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x}) = \left({}^{B}r_{sr,x}(\bar{q}_i,\bar{x}),\ {}^{B}r_{sr,y}(\bar{q}_i,\bar{x}),\ {}^{B}r_{sr,z}(\bar{q}_i,\bar{x})\right)^T,
\qquad
\left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\| = \sqrt{\sum_{k' \in \{x,y,z\}} {}^{B}r_{sr,k'}(\bar{q}_i,\bar{x})^2}
$$

где $k' \in \{x, y, z\}$ — индекс суммирования по координатам.

Производная длины по вектору — по определению строка частных производных длины по координатам вектора:

$$
\frac{\partial \left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\|}{\partial\,{}^{B}\bar{r}_{sr}}
=
\left(
\frac{\partial \left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\|}{\partial\,{}^{B}r_{sr,x}},\
\frac{\partial \left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\|}{\partial\,{}^{B}r_{sr,y}},\
\frac{\partial \left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\|}{\partial\,{}^{B}r_{sr,z}}
\right)
\tag{m.2.2.5}
$$

Частная производная по координате $k$ по правилу дифференцирования сложной функции:

$$
\begin{gathered}
\frac{\partial \left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\|}{\partial\,{}^{B}r_{sr,k}}
= \frac{\partial}{\partial\,{}^{B}r_{sr,k}}\sqrt{\sum_{k' \in \{x,y,z\}} {}^{B}r_{sr,k'}(\bar{q}_i,\bar{x})^2}
= \frac{1}{2\sqrt{\sum_{k' \in \{x,y,z\}} {}^{B}r_{sr,k'}(\bar{q}_i,\bar{x})^2}}\cdot\frac{\partial}{\partial\,{}^{B}r_{sr,k}}\sum_{k' \in \{x,y,z\}} {}^{B}r_{sr,k'}(\bar{q}_i,\bar{x})^2 =\\
= \frac{1}{2\left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\|}\cdot 2\,{}^{B}r_{sr,k}(\bar{q}_i,\bar{x})
= \frac{{}^{B}r_{sr,k}(\bar{q}_i,\bar{x})}{\left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\|}
\end{gathered}
\tag{m.2.2.6}
$$

Подстановка (m.2.2.6) для каждого $k \in \{x, y, z\}$ в (m.2.2.5):

$$
\frac{\partial \left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\|}{\partial\,{}^{B}\bar{r}_{sr}}
= \frac{\left({}^{B}r_{sr,x}(\bar{q}_i,\bar{x}),\ {}^{B}r_{sr,y}(\bar{q}_i,\bar{x}),\ {}^{B}r_{sr,z}(\bar{q}_i,\bar{x})\right)}{\left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})\right\|}
= {}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T
\tag{m.2.2.7}
$$

Шаг 3 — производные координат вектора по координатам центров. Координата $k \in \{x, y, z\}$ вектора (m.2.2.3):

$$
{}^{B}r_{sr,k}(\bar{q}_i,\bar{x}) = {}^{B}p_{F,k}(\bar{q}_i) + \sum_{k' \in \{x,y,z\}} R_{kk'}(\bar{q}_i)\;{}^{F}p_{r,k'} - {}^{B}p_{s,k}
$$

где ${}^{B}p_{F,k}(\bar{q}_i)$ — координата $k$ начала {F} в {B}, мм;
&emsp;&emsp;$R_{kk'}(\bar{q}_i)$ — элемент строки $k$ и столбца $k'$ матрицы ${}^{B}R_F(\bar{q}_i)$;
&emsp;&emsp;${}^{F}p_{r,k'}$ — координата $k'$ центра шарика в {F}, мм;
&emsp;&emsp;${}^{B}p_{s,k}$ — координата $k$ центра сферы в {B}, мм.

Частные производные:

$$
\frac{\partial\,{}^{B}r_{sr,k}(\bar{q}_i,\bar{x})}{\partial\,{}^{F}p_{r,k'}} = R_{kk'}(\bar{q}_i)
\;\Longrightarrow\;
\frac{\partial\,{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})}{\partial\,{}^{F}\bar{p}_r} = {}^{B}R_F(\bar{q}_i),
\qquad
\frac{\partial\,{}^{B}r_{sr,k}(\bar{q}_i,\bar{x})}{\partial\,{}^{B}p_{s,k'}} = \begin{cases} -1, & k = k'\\ 0, & k \ne k' \end{cases}
\;\Longrightarrow\;
\frac{\partial\,{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})}{\partial\,{}^{B}\bar{p}_s} = -I
\tag{m.2.2.8}
$$

где $I$ — единичная матрица $3 \times 3$.

Шаг 4 — подстановка (m.2.2.7) и (m.2.2.8) в (m.2.2.4):

$$
\frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{F}\bar{p}_r} = {}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T\,{}^{B}R_F(\bar{q}_i),
\qquad
\frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{B}\bar{p}_s} = {}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T\,(-I) = -\,{}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T
\tag{m.2.2.9}
$$

Покажем, что ${}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T\,{}^{B}R_F(\bar{q}_i)$ — транспонированный орт ${}^{F}\bar{e}_{sr}(\bar{q}_i,\bar{x})$; координаты из {B} в {F} переводит матрица ${}^{B}R_F(\bar{q}_i)^T$:

$$
{}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T\,{}^{B}R_F(\bar{q}_i) = \left({}^{B}R_F(\bar{q}_i)^T\,{}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})\right)^T = {}^{F}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T
\tag{m.2.2.10}
$$

Итог — строка $i$ матрицы $J$:

$$
\begin{bmatrix} J_{i1} & J_{i2} & J_{i3} \end{bmatrix} = \frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{F}\bar{p}_r} = {}^{F}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T,
\qquad
\begin{bmatrix} J_{i4} & J_{i5} & J_{i6} \end{bmatrix} = \frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{B}\bar{p}_s} = -\,{}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T
$$

$$
\boxed{\begin{bmatrix} J_{i1} & J_{i2} & J_{i3} \end{bmatrix} = {}^{F}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T, \qquad \begin{bmatrix} J_{i4} & J_{i5} & J_{i6} \end{bmatrix} = -\,{}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T}
\tag{m.2.2.11}
$$

#### 2.2.3. Нормальные уравнения нелинейного МНК для калибровки

Получим систему нормальных уравнений нелинейного МНК (m.2.2.2) для калибровки в явном виде и покажем, что она нелинейна относительно $\hat{\bar{x}}$.

Произведение матриц в левой части уравнения (m.2.2.2):

$$
\begin{gathered}
J(\hat{\bar{x}})^T\,\bar{\varepsilon}(\hat{\bar{x}}) = \begin{bmatrix}
J_{11}(\hat{\bar{x}}) & J_{12}(\hat{\bar{x}}) & \cdots & J_{1n}(\hat{\bar{x}})\\
J_{21}(\hat{\bar{x}}) & J_{22}(\hat{\bar{x}}) & \cdots & J_{2n}(\hat{\bar{x}})\\
\vdots & \vdots & \ddots & \vdots\\
J_{m1}(\hat{\bar{x}}) & J_{m2}(\hat{\bar{x}}) & \cdots & J_{mn}(\hat{\bar{x}})
\end{bmatrix}^T
\begin{bmatrix} \varepsilon_1(\hat{\bar{x}}) \\ \varepsilon_2(\hat{\bar{x}}) \\ \vdots \\ \varepsilon_m(\hat{\bar{x}}) \end{bmatrix} =\\
= \begin{bmatrix}
J_{11}(\hat{\bar{x}}) & J_{21}(\hat{\bar{x}}) & \cdots & J_{m1}(\hat{\bar{x}})\\
J_{12}(\hat{\bar{x}}) & J_{22}(\hat{\bar{x}}) & \cdots & J_{m2}(\hat{\bar{x}})\\
\vdots & \vdots & \ddots & \vdots\\
J_{1n}(\hat{\bar{x}}) & J_{2n}(\hat{\bar{x}}) & \cdots & J_{mn}(\hat{\bar{x}})
\end{bmatrix}
\begin{bmatrix} \varepsilon_1(\hat{\bar{x}}) \\ \varepsilon_2(\hat{\bar{x}}) \\ \vdots \\ \varepsilon_m(\hat{\bar{x}}) \end{bmatrix} =\\
= \begin{bmatrix}
J_{11}(\hat{\bar{x}})\,\varepsilon_1(\hat{\bar{x}}) + J_{21}(\hat{\bar{x}})\,\varepsilon_2(\hat{\bar{x}}) + \cdots + J_{m1}(\hat{\bar{x}})\,\varepsilon_m(\hat{\bar{x}})\\
J_{12}(\hat{\bar{x}})\,\varepsilon_1(\hat{\bar{x}}) + J_{22}(\hat{\bar{x}})\,\varepsilon_2(\hat{\bar{x}}) + \cdots + J_{m2}(\hat{\bar{x}})\,\varepsilon_m(\hat{\bar{x}})\\
\vdots\\
J_{1n}(\hat{\bar{x}})\,\varepsilon_1(\hat{\bar{x}}) + J_{2n}(\hat{\bar{x}})\,\varepsilon_2(\hat{\bar{x}}) + \cdots + J_{mn}(\hat{\bar{x}})\,\varepsilon_m(\hat{\bar{x}})
\end{bmatrix} =\\
= \varepsilon_1(\hat{\bar{x}})\begin{bmatrix} J_{11}(\hat{\bar{x}}) \\ J_{12}(\hat{\bar{x}}) \\ \vdots \\ J_{1n}(\hat{\bar{x}}) \end{bmatrix} + \varepsilon_2(\hat{\bar{x}})\begin{bmatrix} J_{21}(\hat{\bar{x}}) \\ J_{22}(\hat{\bar{x}}) \\ \vdots \\ J_{2n}(\hat{\bar{x}}) \end{bmatrix} + \cdots + \varepsilon_m(\hat{\bar{x}})\begin{bmatrix} J_{m1}(\hat{\bar{x}}) \\ J_{m2}(\hat{\bar{x}}) \\ \vdots \\ J_{mn}(\hat{\bar{x}}) \end{bmatrix} =\\
= J_1(\hat{\bar{x}})^T\,\varepsilon_1(\hat{\bar{x}}) + J_2(\hat{\bar{x}})^T\,\varepsilon_2(\hat{\bar{x}}) + \cdots + J_m(\hat{\bar{x}})^T\,\varepsilon_m(\hat{\bar{x}}) = \sum_{i=1}^{m} J_i(\hat{\bar{x}})^T\,\varepsilon_i(\hat{\bar{x}})
\end{gathered}
$$

где $J_i(\hat{\bar{x}})$ — строка $i$ матрицы $J(\hat{\bar{x}})$, вектор-строка размером $1 \times n$.

Подставим (m.2.2.11) при $\bar{x} = \hat{\bar{x}}$ в строку $J_i(\hat{\bar{x}})$:

$$
J_i(\hat{\bar{x}}) = \begin{bmatrix} {}^{F}\bar{e}_{sr}(\bar{q}_i,\hat{\bar{x}})^T \quad & \quad -\,{}^{B}\bar{e}_{sr}(\bar{q}_i,\hat{\bar{x}})^T \end{bmatrix}
$$

Подставим (m.2.2.10) вместо ${}^{F}\bar{e}_{sr}(\bar{q}_i,\hat{\bar{x}})^T$:

$$
J_i(\hat{\bar{x}}) = \begin{bmatrix} {}^{B}\bar{e}_{sr}(\bar{q}_i,\hat{\bar{x}})^T\,{}^{B}R_F(\bar{q}_i) \quad & \quad -\,{}^{B}\bar{e}_{sr}(\bar{q}_i,\hat{\bar{x}})^T \end{bmatrix} = {}^{B}\bar{e}_{sr}(\bar{q}_i,\hat{\bar{x}})^T \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix}
$$

где $I$ — единичная матрица $3 \times 3$;
&emsp;&emsp;$\begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix}$ — матрица размером $3 \times 6$.

Слагаемое $i$ суммы:

$$
J_i(\hat{\bar{x}})^T\,\varepsilon_i(\hat{\bar{x}}) = \left({}^{B}\bar{e}_{sr}(\bar{q}_i,\hat{\bar{x}})^T \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix}\right)^T \varepsilon_i(\hat{\bar{x}}) = \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix}^T {}^{B}\bar{e}_{sr}(\bar{q}_i,\hat{\bar{x}})\,\varepsilon_i(\hat{\bar{x}}) = \begin{bmatrix} {}^{B}R_F(\bar{q}_i)^T \\ -I \end{bmatrix} {}^{B}\bar{e}_{sr}(\bar{q}_i,\hat{\bar{x}})\,\varepsilon_i(\hat{\bar{x}})
$$

Подставим орт и невязку из (m.2.2.3) при $\bar{x} = \hat{\bar{x}}$:

$$
J_i(\hat{\bar{x}})^T\,\varepsilon_i(\hat{\bar{x}}) = \begin{bmatrix} {}^{B}R_F(\bar{q}_i)^T \\ -I \end{bmatrix} \frac{{}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}})}{\left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}})\right\|}\left(\left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}})\right\| - L\right) = \begin{bmatrix} {}^{B}R_F(\bar{q}_i)^T \\ -I \end{bmatrix} {}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}})\left(1 - \frac{L}{\left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}})\right\|}\right)
$$

Подставим это слагаемое в последнюю строку разложения произведения и приравняем к $\bar{0}$ по (m.2.2.2):

$$
\boxed{\sum_{i=1}^{m} \begin{bmatrix} {}^{B}R_F(\bar{q}_i)^T \\ -I \end{bmatrix} {}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}})\left(1 - \frac{L}{\left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}})\right\|}\right) = \bar{0}}
\tag{m.2.2.12}
$$

Подставим $\bar{x} = \hat{\bar{x}}$ в вектор ${}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x})$ из (m.2.2.3):

$$
{}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}}) = {}^{B}\bar{p}_F(\bar{q}_i) + {}^{B}R_F(\bar{q}_i)\,{}^{F}\hat{\bar{p}}_r - {}^{B}\hat{\bar{p}}_s = {}^{B}\bar{p}_F(\bar{q}_i) + \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix} \begin{bmatrix} {}^{F}\hat{\bar{p}}_r \\ {}^{B}\hat{\bar{p}}_s \end{bmatrix} = {}^{B}\bar{p}_F(\bar{q}_i) + \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix} \hat{\bar{x}}
\tag{m.2.2.13}
$$

где ${}^{F}\hat{\bar{p}}_r$, ${}^{B}\hat{\bar{p}}_s$ — элементы 1–3 и 4–6 вектора $\hat{\bar{x}}$ (m.2.1.1): найденные МНК координаты центра шарика в {F} и центра сферы в {B}, мм.

Подставим (m.2.2.13) в норму вектора ${}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}})$:

$$
\begin{gathered}
\left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}})\right\| = \sqrt{{}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}})^T\,{}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}})}
= \sqrt{\left({}^{B}\bar{p}_F(\bar{q}_i) + \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix} \hat{\bar{x}}\right)^T\left({}^{B}\bar{p}_F(\bar{q}_i) + \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix} \hat{\bar{x}}\right)}\\
= \sqrt{{}^{B}\bar{p}_F(\bar{q}_i)^T\,{}^{B}\bar{p}_F(\bar{q}_i) + {}^{B}\bar{p}_F(\bar{q}_i)^T \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix} \hat{\bar{x}} + \hat{\bar{x}}^T \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix}^T {}^{B}\bar{p}_F(\bar{q}_i) + \hat{\bar{x}}^T \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix}^T \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix} \hat{\bar{x}}}\\
= \sqrt{{}^{B}\bar{p}_F(\bar{q}_i)^T\,{}^{B}\bar{p}_F(\bar{q}_i) + 2\,{}^{B}\bar{p}_F(\bar{q}_i)^T \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix} \hat{\bar{x}} + \hat{\bar{x}}^T \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix}^T \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix} \hat{\bar{x}}}
\end{gathered}
\tag{m.2.2.14}
$$

Посмотрим, каким станет уравнение (m.2.2.12) при $L = 0$. Подставим $L = 0$ в (m.2.2.12):

$$
\sum_{i=1}^{m} \begin{bmatrix} {}^{B}R_F(\bar{q}_i)^T \\ -I \end{bmatrix} {}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}})\left(1 - \frac{0}{\left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}})\right\|}\right) = \sum_{i=1}^{m} \begin{bmatrix} {}^{B}R_F(\bar{q}_i)^T \\ -I \end{bmatrix} {}^{B}\bar{r}_{sr}(\bar{q}_i,\hat{\bar{x}}) = \bar{0}
$$

Подставим в это уравнение вектор (m.2.2.13) и раскроем скобки:

$$
\begin{gathered}
\sum_{i=1}^{m} \begin{bmatrix} {}^{B}R_F(\bar{q}_i)^T \\ -I \end{bmatrix} \left({}^{B}\bar{p}_F(\bar{q}_i) + \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix} \hat{\bar{x}}\right) =\\
= \sum_{i=1}^{m} \begin{bmatrix} {}^{B}R_F(\bar{q}_i)^T \\ -I \end{bmatrix} {}^{B}\bar{p}_F(\bar{q}_i) + \left(\sum_{i=1}^{m} \begin{bmatrix} {}^{B}R_F(\bar{q}_i)^T \\ -I \end{bmatrix} \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix}\right) \hat{\bar{x}} = \bar{0}
\end{gathered}
$$

$$
\boxed{G\,\hat{\bar{x}} = \bar{b}},
\qquad
G = \sum_{i=1}^{m} \begin{bmatrix} {}^{B}R_F(\bar{q}_i)^T \\ -I \end{bmatrix} \begin{bmatrix} {}^{B}R_F(\bar{q}_i) \quad & \quad -I \end{bmatrix},
\qquad
\bar{b} = -\sum_{i=1}^{m} \begin{bmatrix} {}^{B}R_F(\bar{q}_i)^T \\ -I \end{bmatrix} {}^{B}\bar{p}_F(\bar{q}_i)
$$

где $G$ — матрица размером $6 \times 6$;
&emsp;&emsp;$\bar{b}$ — вектор-столбец размером $6 \times 1$, мм.

Величины $G$ и $\bar{b}$ зависят только от измеренных углов сочленений $\bar{q}_i$ и не зависят от $\hat{\bar{x}}$, поэтому $G\,\hat{\bar{x}} = \bar{b}$ — система $n = 6$ линейных уравнений относительно $n$ элементов $\hat{\bar{x}}$.

При $L \ne 0$ множитель в скобках (m.2.2.12) содержит норму вектора (m.2.2.14), и (m.2.2.12) не приводится к виду $G\,\hat{\bar{x}} = \bar{b}$. Поэтому (m.2.2.12) можно решить методом Гаусса — Ньютона.

#### 2.2.4. Решение нормальных уравнений методом Гаусса — Ньютона

Получим нормальные уравнения линеаризованной задачи для приращения вектора параметров на итерации метода Гаусса — Ньютона и докажем, что приращение единственно тогда и только тогда, когда столбцы матрицы Якоби линейно независимы.

На итерации $\nu$ вектор невязок вблизи текущего приближения $\bar{x}_\nu$ приближается многочленом Тейлора первой степени:

$$
\bar{\varepsilon}(\bar{x}_\nu + \Delta\bar{x}) \approx \bar{\varepsilon}(\bar{x}_\nu) + J(\bar{x}_\nu)\,\Delta\bar{x}
$$

где $\nu = 0, 1, 2, \ldots$ — номер итерации;
&emsp;&emsp;$\bar{x}_\nu$ — приближение на итерации $\nu$, $\bar{x}_0$ — начальное приближение;
&emsp;&emsp;$\Delta\bar{x} \in \mathbb{R}^n$ — приращение, вектор-столбец размером $n \times 1$;
&emsp;&emsp;$\Delta x_j$ — элемент $j$ вектора $\Delta\bar{x}$;
&emsp;&emsp;$J(\bar{x}_\nu)$ — матрица Якоби (m.2.2.1) при $\bar{x}_\nu$, размером $m \times n$.

Ниже для краткости $\bar{\varepsilon}_\nu = \bar{\varepsilon}(\bar{x}_\nu)$ и $J_\nu = J(\bar{x}_\nu)$.

Сумма квадратов невязок при $\bar{x}_\nu + \Delta\bar{x}$ в линейном приближении:

$$
\begin{gathered}
\Phi(\bar{x}_\nu + \Delta\bar{x}) \approx \Phi_\nu(\Delta\bar{x}) = \left(\bar{\varepsilon}_\nu + J_\nu\,\Delta\bar{x}\right)^T\left(\bar{\varepsilon}_\nu + J_\nu\,\Delta\bar{x}\right)
= \bar{\varepsilon}_\nu^T\bar{\varepsilon}_\nu + \bar{\varepsilon}_\nu^T J_\nu\,\Delta\bar{x} + \Delta\bar{x}^T J_\nu^T\,\bar{\varepsilon}_\nu + \Delta\bar{x}^T J_\nu^T J_\nu\,\Delta\bar{x} =\\
= \bar{\varepsilon}_\nu^T\bar{\varepsilon}_\nu + 2\,\Delta\bar{x}^T J_\nu^T\,\bar{\varepsilon}_\nu + \Delta\bar{x}^T J_\nu^T J_\nu\,\Delta\bar{x}
\end{gathered}
\tag{m.2.2.15}
$$

где $\Phi_\nu(\Delta\bar{x})$ — сумма квадратов линеаризованных невязок на итерации $\nu$, мм².

Приращение выбирается из необходимого условия минимума $\Phi_\nu(\Delta\bar{x})$: частные производные $\Phi_\nu$ по всем элементам $\Delta x_j$ равны нулю. Частная производная вектора $\Delta\bar{x}$ по его элементу $j$ — орт $\bar{e}_j$ оси $j$ пространства $\mathbb{R}^n$. Матрица $J_\nu^T J_\nu$ симметрична:

$$
\left(J_\nu^T J_\nu\right)^T = J_\nu^T\left(J_\nu^T\right)^T = J_\nu^T J_\nu
$$

Частные производные слагаемых (m.2.2.15) по правилу дифференцирования произведения:

$$
\begin{gathered}
\frac{\partial}{\partial \Delta x_j}\left(\bar{\varepsilon}_\nu^T\bar{\varepsilon}_\nu\right) = 0\\
\frac{\partial}{\partial \Delta x_j}\left(2\,\Delta\bar{x}^T J_\nu^T\,\bar{\varepsilon}_\nu\right) = 2\,\bar{e}_j^T J_\nu^T\,\bar{\varepsilon}_\nu = 2\left(J_\nu^T\,\bar{\varepsilon}_\nu\right)_j\\
\frac{\partial}{\partial \Delta x_j}\left(\Delta\bar{x}^T J_\nu^T J_\nu\,\Delta\bar{x}\right) = \bar{e}_j^T J_\nu^T J_\nu\,\Delta\bar{x} + \Delta\bar{x}^T J_\nu^T J_\nu\,\bar{e}_j = \bar{e}_j^T J_\nu^T J_\nu\,\Delta\bar{x} + \bar{e}_j^T \left(J_\nu^T J_\nu\right)^T \Delta\bar{x} =\\
= 2\,\bar{e}_j^T J_\nu^T J_\nu\,\Delta\bar{x} = 2\left(J_\nu^T J_\nu\,\Delta\bar{x}\right)_j
\end{gathered}
$$

где $j = 1 \ldots n$.

Равенство нулю всех $n$ частных производных и нормальные уравнения:

$$
2\left(J_\nu^T\,\bar{\varepsilon}_\nu\right)_j + 2\left(J_\nu^T J_\nu\,\Delta\bar{x}\right)_j = 0, \quad j = 1 \ldots n
\;\Longleftrightarrow\;
2\,J_\nu^T\,\bar{\varepsilon}_\nu + 2\,J_\nu^T J_\nu\,\Delta\bar{x} = \bar{0}
\;\Longleftrightarrow\;
\left(J_\nu^T J_\nu\right)\Delta\bar{x} = -J_\nu^T\,\bar{\varepsilon}_\nu
$$

$$
\boxed{\left(J_\nu^T J_\nu\right)\Delta\bar{x} = -J_\nu^T\,\bar{\varepsilon}_\nu}
\tag{m.2.2.16}
$$

Нормальные уравнения линеаризованной задачи — система из $n$ линейных уравнений относительно $n$ элементов $\Delta\bar{x}$ с матрицей $J_\nu^T J_\nu$ размером $n \times n$. Найденное приращение даёт следующее приближение:

$$
\bar{x}_{\nu+1} = \bar{x}_\nu + \Delta\bar{x}
$$

Если $\Delta\bar{x} = \bar{0}$ (итерации сошлись: $\bar{x}_{\nu+1} = \bar{x}_\nu$), нормальные уравнения (m.2.2.16) совпадают с уравнением (m.2.2.2) при $\hat{\bar{x}} = \bar{x}_\nu$.

Докажем, что при линейно независимых столбцах $J_\nu$ приращение единственно, а при линейно зависимых — нет.

Столбцы линейно независимы, $\operatorname{rank} J_\nu = n$. Пусть $J_\nu^T J_\nu\,\bar{\alpha} = \bar{0}$ для некоторого $\bar{\alpha} \in \mathbb{R}^n$. Тогда

$$
0 = \bar{\alpha}^T J_\nu^T J_\nu\,\bar{\alpha} = \left(J_\nu\,\bar{\alpha}\right)^T\left(J_\nu\,\bar{\alpha}\right) = \left\|J_\nu\,\bar{\alpha}\right\|^2
\;\Longrightarrow\; J_\nu\,\bar{\alpha} = \bar{0}
\;\Longrightarrow\; \bar{\alpha} = \bar{0}
$$

где $\bar{\alpha}$ — вектор-столбец размером $n \times 1$.

Квадратная матрица, для которой из $J_\nu^T J_\nu\,\bar{\alpha} = \bar{0}$ следует $\bar{\alpha} = \bar{0}$, обратима, поэтому приращение единственно:

$$
\boxed{\Delta\bar{x} = -\left(J_\nu^T J_\nu\right)^{-1} J_\nu^T\,\bar{\varepsilon}_\nu}
$$

Столбцы линейно зависимы:

$$
\exists\, \bar{\alpha} \in \mathbb{R}^n \setminus \{\bar{0}\}:\; J_\nu\,\bar{\alpha} = \sum_{j=1}^{n} \alpha_j\,\bar{c}_j = \bar{0}
\tag{m.2.2.17}
$$

где $\bar{c}_j$ — столбец $j$ матрицы $J_\nu$, вектор-столбец размером $m \times 1$;
&emsp;&emsp;$\alpha_j$ — элемент $j$ вектора $\bar{\alpha}$.

Тогда $J_\nu^T J_\nu$ вырождена, и вместе с любым решением $\Delta\bar{x}$ нормальных уравнений (m.2.2.16) решением является $\Delta\bar{x} + \theta\,\bar{\alpha}$ при любом $\theta \in \mathbb{R}$:

$$
J_\nu^T J_\nu\,\bar{\alpha} = J_\nu^T\left(J_\nu\,\bar{\alpha}\right) = J_\nu^T\,\bar{0} = \bar{0},
\qquad
\left(J_\nu^T J_\nu\right)\left(\Delta\bar{x} + \theta\,\bar{\alpha}\right) = \left(J_\nu^T J_\nu\right)\Delta\bar{x} + \theta\,J_\nu^T J_\nu\,\bar{\alpha} = -J_\nu^T\,\bar{\varepsilon}_\nu
$$

Все эти приращения дают одну и ту же сумму квадратов линеаризованных невязок, потому что $J_\nu\,\bar{\alpha} = \bar{0}$:

$$
\Phi_\nu(\Delta\bar{x} + \theta\,\bar{\alpha}) = \left\|\bar{\varepsilon}_\nu + J_\nu\,\Delta\bar{x} + \theta\,J_\nu\,\bar{\alpha}\right\|^2 = \left\|\bar{\varepsilon}_\nu + J_\nu\,\Delta\bar{x}\right\|^2 = \Phi_\nu(\Delta\bar{x})
\tag{m.2.2.18}
$$

Итог: при линейно зависимых столбцах решения нормальных уравнений на итерации $\nu$ образуют не одну точку, а прямую $\Delta\bar{x} + \theta\,\bar{\alpha}$, $\theta \in \mathbb{R}$, и $\Phi_\nu$ на ней одинакова. Комбинация параметров вдоль $\bar{\alpha}$ неидентифицируема в линейном приближении; что одинакова и сама $\Phi$, для нарушенных условий на набор касаний показано в разделе 2.2.5:

$$
\boxed{\operatorname{rank} J_\nu < n \;\Longrightarrow\; J_\nu^T J_\nu \text{ вырождена} \;\Longrightarrow\; \text{приращение } \Delta\bar{x} \text{ не единственно},
\qquad
\operatorname{rank} J_\nu = n \;\Longrightarrow\; J_\nu^T J_\nu \text{ обратима} \;\Longrightarrow\; \text{приращение единственно}}
\tag{m.2.2.19}
$$

#### 2.2.5. Условия на набор касаний и ранг матрицы Якоби

Докажем, что при нарушении любого из трёх условий на набор касаний ранг матрицы Якоби меньше 6, и найдём для каждого условия неидентифицируемую комбинацию параметров.

По (m.2.2.19) приращение метода Гаусса — Ньютона единственно при $\operatorname{rank} J = n = 6$. Если выполнено (m.2.2.17), то по (m.2.2.18) приращения, отличающиеся на $\theta\,\bar{\alpha}$, $\theta \in \mathbb{R}$, дают одну и ту же сумму квадратов линеаризованных невязок, и комбинация параметров вдоль $\bar{\alpha}$ неидентифицируема.

Условие 1: касаний не меньше, чем параметров, $m \ge n = 6$.

Докажем, что при $m < 6$ ранг $J$ меньше 6. Ранг матрицы равен числу её линейно независимых строк и числу её линейно независимых столбцов; у $J$ всего $m$ строк:

$$
\operatorname{rank} J \le m < 6 = n
$$

Число векторов в фундаментальной системе решений однородной системы из (m.2.2.17) равно $n - \operatorname{rank} J$:

$$
\boxed{n - \operatorname{rank} J \ge 6 - m}
$$

поэтому неидентифицируемы не меньше $6 - m$ линейно независимых комбинаций параметров.

Условие 2: ориентация фланца меняется между касаниями.

Докажем, что при одной ориентации у всех касаний ранг $J$ не больше 3. При одной ориентации поворот фланца одинаков у всех касаний:

$$
{}^{B}R_F(\bar{q}_i) = {}^{B}R_F, \qquad i = 1 \ldots m
$$

где ${}^{B}R_F$ — общая для всех касаний матрица поворота фланца.

Второе равенство (m.2.2.9), умноженное на $-1$:

$$
{}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T = -\,\frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{B}\bar{p}_s}
\tag{m.2.2.20}
$$

Подставим в первое равенство (m.2.2.9) сначала общую матрицу поворота, затем (m.2.2.20):

$$
\frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{F}\bar{p}_r} = {}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T\,{}^{B}R_F(\bar{q}_i) = {}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T\,{}^{B}R_F = -\,\frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{B}\bar{p}_s}\,{}^{B}R_F,
\qquad i = 1 \ldots m
$$

Поэлементно, для каждого $k' \in \{x, y, z\}$:

$$
\frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{F}p_{r,k'}} = -\sum_{k \in \{x,y,z\}} \frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{B}p_{s,k}}\,R_{kk'}
\;\Longrightarrow\;
\bar{c}_{k'} = -\sum_{k \in \{x,y,z\}} R_{kk'}\,\bar{c}_{3+k}
$$

где $R_{kk'}$ — элемент строки $k$ и столбца $k'$ матрицы поворота фланца, одинаковый у всех касаний;
&emsp;&emsp;$\bar{c}_j$ — столбец $j$ матрицы $J$, как в (m.2.2.17);
&emsp;&emsp;$\bar{c}_{k'}$, $\bar{c}_{3+k}$ — столбцы производных по координате $k'$ центра шарика и по координате $k$ центра сферы; в их индексах x, y, z заменены номерами 1, 2, 3, как номера параметров $j$ в (m.2.1.1).

Поэтому $\operatorname{rank} J \le 3$ при любом числе касаний, и условие 1 необходимо, но недостаточно.

Та же связь столбцов в виде однородной системы из (m.2.2.17):

$$
\bar{c}_{k'} + \sum_{k \in \{x,y,z\}} R_{kk'}\,\bar{c}_{3+k} = \bar{0}
\;\Longleftrightarrow\;
J\,\bar{\alpha}_{k'} = \bar{0},
\qquad
\bar{\alpha}_{k'} = \begin{bmatrix} {}^{F}\bar{e}_{k'} \\ {}^{B}R_F\,{}^{F}\bar{e}_{k'} \end{bmatrix} = \begin{bmatrix} {}^{F}\bar{e}_{k'} \\ {}^{B}\bar{e}_{k'} \end{bmatrix},
\qquad k' \in \{x, y, z\}
$$

$$
\boxed{J\,\bar{\alpha}_{k'} = \bar{0}, \qquad \bar{\alpha}_{k'} = \begin{bmatrix} {}^{F}\bar{e}_{k'} \\ {}^{B}\bar{e}_{k'} \end{bmatrix}, \qquad k' \in \{x, y, z\}}
\tag{m.2.2.21}
$$

где $\bar{\alpha}_{k'} \in \mathbb{R}^6$ — вектор-столбец;
&emsp;&emsp;${}^{F}\bar{e}_{k'}$ — орт оси $k'$ системы {F}, выраженный в {F};
&emsp;&emsp;${}^{B}\bar{e}_{k'}$ — тот же орт, выраженный в {B}.

Изменение параметров на $\theta\,\bar{\alpha}_{k'}$ не меняет вектор ${}^{B}\bar{r}_{sr}$ из (m.2.2.3) ни при одном касании и ни при каком $\theta$, не только в линейном приближении:

$$
\begin{gathered}
{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x} + \theta\,\bar{\alpha}_{k'}) = {}^{B}\bar{p}_F(\bar{q}_i) + {}^{B}R_F\left({}^{F}\bar{p}_r + \theta\,{}^{F}\bar{e}_{k'}\right) - \left({}^{B}\bar{p}_s + \theta\,{}^{B}\bar{e}_{k'}\right) =\\
= {}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x}) + \theta\left({}^{B}R_F\,{}^{F}\bar{e}_{k'} - {}^{B}\bar{e}_{k'}\right) = {}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x}),
\qquad \theta \in \mathbb{R}, \quad i = 1 \ldots m
\end{gathered}
$$

Поэтому невязки при $\bar{x}$ и $\bar{x} + \theta\,\bar{\alpha}_{k'}$ одинаковы. Неидентифицируемы три комбинации: сдвиг центра шарика на $\theta$ вдоль оси $k'$ системы {F} вместе со сдвигом центра сферы на $\theta$ вдоль той же оси, выраженной в {B}.

Условие 3: орт оси стилуса в {B} меняет направление между касаниями, то есть повороты происходят не только вокруг оси стилуса.

Докажем, что при поворотах только вокруг оси стилуса ранг $J$ не больше 5. Орт оси стилуса, выраженный в {B} при касании $i$:

$$
{}^{B}\bar{e}_z(\bar{q}_i) = {}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{e}_z
\tag{m.2.2.22}
$$

Если повороты происходят только вокруг оси стилуса, этот орт одинаков у всех касаний:

$$
{}^{B}\bar{e}_z(\bar{q}_i) = {}^{B}\bar{e}_z, \qquad i = 1 \ldots m
$$

Подставим в производную по координате z центра шарика по очереди первое равенство (m.2.2.9), (m.2.2.22), одинаковый у всех касаний орт ${}^{B}\bar{e}_z$ и (m.2.2.20):

$$
\frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{F}p_{r,z}} = \frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{F}\bar{p}_r}\,{}^{F}\bar{e}_z = {}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T\,{}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{e}_z = {}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T\,{}^{B}\bar{e}_z(\bar{q}_i) = {}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})^T\,{}^{B}\bar{e}_z = -\,\frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{B}\bar{p}_s}\,{}^{B}\bar{e}_z
$$

Коэффициенты — координаты орта оси стилуса в {B} — одинаковы у всех касаний, поэтому столбец 3 — линейная комбинация столбцов 4–6, и ранг $J$ не больше 5. Для координат x и y так не получается: выраженные в {B} орты осей x и y при повороте вокруг оси стилуса меняются от касания к касанию,

$$
{}^{B}\bar{e}_x(\bar{q}_i) = {}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{e}_x \ne \text{const},
$$

поэтому коэффициенты комбинации разные в разных строках, и столбцы 1 и 2 линейными комбинациями столбцов 4–6 не являются.

Связь столбца 3 со столбцами 4–6 в виде однородной системы из (m.2.2.17):

$$
\bar{c}_3 + \sum_{k \in \{x,y,z\}} {}^{B}e_{z,k}\,\bar{c}_{3+k} = \bar{0}
\;\Longleftrightarrow\;
J\,\bar{\alpha}_z = \bar{0}
$$

$$
\boxed{J\,\bar{\alpha}_z = \bar{0}}
$$

где $\bar{\alpha}_z$ — вектор (m.2.2.21) при $k' = z$;
&emsp;&emsp;${}^{B}e_{z,k}$ — координата $k$ орта ${}^{B}\bar{e}_z$;
&emsp;&emsp;$\bar{c}_3$ — столбец производных по координате z центра шарика.

Подставим $\bar{x} + \theta\,\bar{\alpha}_z$ в вектор ${}^{B}\bar{r}_{sr}$ из (m.2.2.3), затем (m.2.2.22) с одинаковым у всех касаний ${}^{B}\bar{e}_z$:

$$
\begin{gathered}
{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x} + \theta\,\bar{\alpha}_z) = {}^{B}\bar{p}_F(\bar{q}_i) + {}^{B}R_F(\bar{q}_i)\left({}^{F}\bar{p}_r + \theta\,{}^{F}\bar{e}_z\right) - \left({}^{B}\bar{p}_s + \theta\,{}^{B}\bar{e}_z\right) =\\
= {}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x}) + \theta\left({}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{e}_z - {}^{B}\bar{e}_z\right) = {}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x}),
\qquad \theta \in \mathbb{R}, \quad i = 1 \ldots m
\end{gathered}
$$

Неидентифицируема одна комбинация: сдвиг центра шарика на $\theta$ вдоль оси стилуса вместе со сдвигом центра сферы на $\theta$ вдоль оси стилуса, выраженной в {B}.

Пример: две ориентации фланца, в которых стилус направлен вертикально вниз и которые различаются поворотом на 180° вокруг оси стилуса:

$$
{}^{B}R_{F,1} = R_x(180°), \qquad {}^{B}R_{F,2} = R_z(180°)\,R_x(180°)
$$

где ${}^{B}R_{F,1}$, ${}^{B}R_{F,2}$ — повороты фланца в первой и второй ориентации.

Ось стилуса в осях {F} — вектор (0, 0, 1). Его образ в {B}:

| <nobr>Множитель</nobr>                                           | <nobr>Вектор</nobr> <nobr>после</nobr> <nobr>множителя</nobr>                                                     |
| --------------------------------------------------- | -------------------------------------------------------------------------- |
| поворот вокруг x на 180°                            | (0, 0, −1)                                                                 |
| поворот вокруг z на 180° (только вторая ориентация) | (0, 0, −1): поворот вокруг оси z системы {B} не меняет вертикальный вектор |

Итог: при нарушении любого из условий 1–3 выполнено (m.2.2.17) и $\operatorname{rank} J < 6$, поэтому условия 1–3 необходимы. Что их выполнение даёт $\operatorname{rank} J = 6$, здесь не доказано; для конкретного набора касаний это проверяется вычислением ранга $J(\hat{\bar{x}})$.

| №   | <nobr>Условие</nobr>                                                  | <nobr>Нарушение</nobr>                          | <nobr>Ранг</nobr> $J$              | <nobr>Неидентифицируемая</nobr> <nobr>комбинация:</nobr> <nobr>вектор</nobr> $\bar{\alpha}$ <nobr>из</nobr> (m.2.2.17)                                                                |
| --- | -------------------------------------------------------- | ---------------------------------- | --------------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| 1   | касаний не меньше, чем параметров: $m \ge n = 6$         | $m < 6$                            | $\le m$               | не меньше $6 - m$ линейно независимых векторов $\bar{\alpha}$                                                                   |
| 2   | ориентация фланца меняется между касаниями               | одна ориентация у всех касаний     | $\le 3$ при любом $m$ | $\bar{\alpha}_{k'}$, $k' \in \{x, y, z\}$: сдвиг центра шарика вдоль оси $k'$ системы {F} и центра сферы вдоль той же оси в {B} |
| 3   | орт оси стилуса в {B} меняет направление между касаниями | повороты только вокруг оси стилуса | $\le 5$ при любом $m$ | $\bar{\alpha}_{k'}$ при $k' = z$: сдвиг центра шарика и центра сферы вдоль оси стилуса                                          |

### 2.3. Случай 1: невязки после оценивания и шум

Невязка — это просто разность рассчитанного и требуемого расстояния; сама по себе она не хороша и не плоха. Вопрос в другом: чем она вызвана. Одна невязка, например $\varepsilon_5 = -0{,}010$ мм из раздела 2.1, этого не говорит: −0,010 мм может быть шумом, а может быть следом того, что модель не учитывает (выброс отдельного касания, перебег шарика после касания из случая 2, зависимость хода до срабатывания от направления из случая 3). Чтобы отличить одно от другого, нужно знать, какие невязки даёт один только шум. Ниже это выводится последовательно.

Понятия:

| <nobr>Понятие</nobr>                               | <nobr>Определение</nobr>                                                                                                                                                                                                                                                                                                  |
| ------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| шум                                   | часть невязки, которая при повторении того же касания (та же точка сферы, та же ориентация, та же скорость поиска) принимает другое значение; для отдельного касания её предсказать нельзя                                                                                                                   |
| систематическая часть                 | часть невязки, которая при повторении того же касания повторяется                                                                                                                                                                                                                                            |
| математическое ожидание $E[\cdot]$    | среднее значение величины по бесконечному числу повторений той же калибровки                                                                                                                                                                                                                                 |
| СКО (среднеквадратическое отклонение) | мера разброса случайной величины: корень из математического ожидания квадрата её отклонения от среднего. Для равномерного распределения на отрезке длиной $a$ СКО равно $a / \sqrt{12}$. Для нормального распределения около 68 % значений лежат в пределах ±1 СКО от среднего и 99,73 % — в пределах ±3 СКО |

#### 2.3.1. Невязки при истинных параметрах равны шуму

Пример с одной величиной ($n = 1$): величина измеряется $m$ раз.

$$
y_i = y^{*} + w_i, \qquad \varepsilon_i(y) = y_i - y, \qquad i = 1 \ldots m
\tag{m.2.3.1}
$$

где $y_i$ — результат измерения $i$;
&emsp;&emsp;$y^{*}$ — истинное значение измеряемой величины;
&emsp;&emsp;$w_i$ — шум измерения $i$;
&emsp;&emsp;$\varepsilon_i(y)$ — невязка измерения $i$ при значении $y$.

Невязка при истинном значении:

$$
\varepsilon_i(y^{*}) = y_i - y^{*} = \left(y^{*} + w_i\right) - y^{*} = w_i
$$

Сумма квадратов невязок при истинном значении:

$$
\sum_{i=1}^{m}\varepsilon_i(y^{*})^2 = \sum_{i=1}^{m}(y_i - y^{*})^2 = \sum_{i=1}^{m} w_i^2
$$

$$
\boxed{\sum_{i=1}^{m}\varepsilon_i(y^{*})^2 = \sum_{i=1}^{m} w_i^2}
$$

#### 2.3.2. Невязки после оценивания

Продолжение примера раздела 2.3.1.

МНК-оценка $\hat{y}$ — значение $y$, при котором сумма квадратов невязок минимальна. Производная суммы квадратов невязок по $y$:

$$
\begin{gathered}
\frac{d}{dy}\sum_{i=1}^{m}\varepsilon_i(y)^2 = \frac{d}{dy}\sum_{i=1}^{m}(y_i - y)^2 = \sum_{i=1}^{m}\frac{d}{dy}(y_i - y)^2 = \sum_{i=1}^{m} 2\,(y_i - y)\cdot\frac{d}{dy}(y_i - y) = \sum_{i=1}^{m} 2\,(y_i - y)\cdot(-1) =\\
= -2\sum_{i=1}^{m}(y_i - y) = -2\left(\sum_{i=1}^{m} y_i - \sum_{i=1}^{m} y\right) = -2\left(\sum_{i=1}^{m} y_i - \underbrace{(y + y + \ldots + y)}_{m \text{ слагаемых}}\right) = -2\left(\sum_{i=1}^{m} y_i - m\,y\right)
\end{gathered}
$$

Приравнивание производной к нулю:

$$
-2\left(\sum_{i=1}^{m} y_i - m\,y\right) = 0
\;\Longrightarrow\; \sum_{i=1}^{m} y_i - m\,y = 0
\;\Longrightarrow\; m\,y = \sum_{i=1}^{m} y_i
\;\Longrightarrow\; y = \frac{1}{m}\sum_{i=1}^{m} y_i
$$

Вторая производная:

$$
\frac{d^2}{dy^2}\sum_{i=1}^{m}\varepsilon_i(y)^2 = \frac{d}{dy}\left[-2\left(\sum_{i=1}^{m} y_i - m\,y\right)\right] = \frac{d}{dy}\left(-2\sum_{i=1}^{m} y_i + 2m\,y\right) = 0 + 2m = 2m > 0
$$

Вторая производная положительна, поэтому найденное значение — минимум:

$$
\boxed{\hat{y} = \frac{1}{m}\sum_{i=1}^{m} y_i = \bar{y}}
$$

где $\bar{y}$ — среднее $m$ измерений $y_1 \ldots y_m$.

Докажем, что сумма квадратов невязок после оценивания меньше суммы квадратов шума ровно на $m\,(\bar{y} - y^{*})^2$.

$$
\begin{gathered}
\sum_{i=1}^{m}\varepsilon_i(\hat{y})^2 = \sum_{i=1}^{m}(y_i - \hat{y})^2 = \sum_{i=1}^{m}(y_i - \bar{y})^2 = \sum_{i=1}^{m}\Big((y_i - y^{*}) - (\bar{y} - y^{*})\Big)^2 =\\
= \sum_{i=1}^{m}\Big[(y_i - y^{*})^2 - 2\,(y_i - y^{*})(\bar{y} - y^{*}) + (\bar{y} - y^{*})^2\Big]
= \sum_{i=1}^{m}(y_i - y^{*})^2 - 2\,(\bar{y} - y^{*})\sum_{i=1}^{m}(y_i - y^{*}) + \sum_{i=1}^{m}(\bar{y} - y^{*})^2
\end{gathered}
\tag{m.2.3.2}
$$

Сумма $\sum_{i=1}^{m}(y_i - y^{*})$ из второго слагаемого:

$$
\bar{y} = \frac{1}{m}\sum_{i=1}^{m} y_i \;\Longrightarrow\; \sum_{i=1}^{m} y_i = m\,\bar{y},
\qquad
\sum_{i=1}^{m}(y_i - y^{*}) = \sum_{i=1}^{m} y_i - \underbrace{(y^{*} + \ldots + y^{*})}_{m \text{ слагаемых}} = m\,\bar{y} - m\,y^{*} = m\,(\bar{y} - y^{*})
\tag{m.2.3.3}
$$

Сумма $\sum_{i=1}^{m}(\bar{y} - y^{*})^2$ из третьего слагаемого:

$$
\sum_{i=1}^{m}(\bar{y} - y^{*})^2
= \underbrace{(\bar{y} - y^{*})^2 + (\bar{y} - y^{*})^2 + \ldots + (\bar{y} - y^{*})^2}_{m \text{ слагаемых}}
= m\,(\bar{y} - y^{*})^2
\tag{m.2.3.4}
$$

Подставим (m.2.3.3) и (m.2.3.4) в (m.2.3.2), затем (m.2.3.1):

$$
\begin{gathered}
\sum_{i=1}^{m}\varepsilon_i(\hat{y})^2 = \sum_{i=1}^{m}(y_i - y^{*})^2 - 2\,(\bar{y} - y^{*})\cdot m\,(\bar{y} - y^{*}) + m\,(\bar{y} - y^{*})^2 = \sum_{i=1}^{m}(y_i - y^{*})^2 - m\,(\bar{y} - y^{*})^2 =\\
= \sum_{i=1}^{m} w_i^2 - m\,(\bar{y} - y^{*})^2 \;\le\; \sum_{i=1}^{m} w_i^2
\end{gathered}
$$

$$
\boxed{\sum_{i=1}^{m}\varepsilon_i(\hat{y})^2 = \sum_{i=1}^{m} w_i^2 - m\,(\bar{y} - y^{*})^2}
\tag{m.2.3.5}
$$

Докажем, что $\bar{y} - y^{*}$ равно среднему шуму, а каждая невязка после оценивания — шуму измерения минус средний шум.

$$
\bar{y} - y^{*} = \frac{1}{m}\sum_{i=1}^{m} y_i - \frac{1}{m}\,m\,y^{*} = \frac{1}{m}\sum_{i=1}^{m} y_i - \frac{1}{m}\sum_{i=1}^{m} y^{*} = \frac{1}{m}\sum_{i=1}^{m} \left(y_i - y^{*}\right) = \frac{1}{m}\sum_{i=1}^{m} w_i = \langle w \rangle
$$

где $\langle w \rangle$ — средний шум $m$ измерений, мм.

Отсюда

$$
m\,(\bar{y} - y^{*})^2 = m\,\langle w \rangle^2,
\qquad
\varepsilon_i(\hat{y}) = y_i - \hat{y} = y_i - \bar{y} = \left(y^{*} + w_i\right) - \left(y^{*} + \langle w \rangle\right) = w_i - \langle w \rangle
\tag{m.2.3.6}
$$

Подставим второе равенство (m.2.3.6) в сумму квадратов невязок после оценивания:

$$
\begin{gathered}
\sum_{i=1}^{m} \varepsilon_i(\hat{y})^2 = \sum_{i=1}^{m} \left(w_i - \langle w \rangle\right)^2 = \sum_{i=1}^{m} \left[w_i^2 - 2\,w_i\,\langle w \rangle + \langle w \rangle^2\right]
= \sum_{i=1}^{m} w_i^2 - 2\,\langle w \rangle \sum_{i=1}^{m} w_i + m\,\langle w \rangle^2 =\\
= \sum_{i=1}^{m} w_i^2 - 2\,\langle w \rangle\cdot m\,\langle w \rangle + m\,\langle w \rangle^2
= \sum_{i=1}^{m} w_i^2 - m\,\langle w \rangle^2
\end{gathered}
$$

$$
\boxed{\sum_{i=1}^{m} \varepsilon_i(\hat{y})^2 = \sum_{i=1}^{m} w_i^2 - m\,\langle w \rangle^2}
$$

Результат совпадает с (m.2.3.5) по первому равенству (m.2.3.6).

Крайний случай: шум всех $m$ измерений одинаковый, $w_i = 0{,}003$ мм:

$$
\langle w \rangle = \frac{1}{m}\sum_{i=1}^{m} 0{,}003 = 0{,}003\ \text{мм},
\qquad
\bar{y} - y^{*} = \langle w \rangle = 0{,}003\ \text{мм},
\qquad
\varepsilon_i(\hat{y}) = w_i - \langle w \rangle = 0{,}003 - 0{,}003 = 0
$$

#### 2.3.3. Насколько меньше: делитель m - n

$$
\begin{aligned}
\bar{x}^{*} &\ \text{— истинные значения искомых параметров; неизвестны}\\
\hat{\bar{x}} &\ \text{— вектор параметров, найденный МНК}
\end{aligned}
$$

Докажем, что для калибровки невязка при истинных параметрах тоже равна шуму. По допущению случая 1:

$$
\left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x}^{*})\right\| = L + w_i
\quad\Longrightarrow\quad
\varepsilon_i(\bar{x}^{*}) = \left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x}^{*})\right\| - L = L + w_i - L = w_i
\tag{m.2.3.7}
$$

где $\left\|{}^{B}\bar{r}_{sr}(\bar{q}_i,\bar{x}^{*})\right\|$ — длина вектора (m.2.2.3) при истинных параметрах, мм;
&emsp;&emsp;$w_i$ — шум касания $i$, мм.

Допущения о шуме: среднее 0; у всех касаний одно и то же СКО $\sigma$ (истинное СКО шума одного касания, мм); шум разных касаний независим. Если бы $\bar{x}^{*}$ были известны, $\sigma$ оценивалось бы прямо по определению СКО (среднее шума 0):

$$
\sigma \approx \sqrt{\frac{1}{m}\sum_{i=1}^{m} w_i^2}
\tag{m.2.3.8}
$$

Ниже $\Phi(\bar{x})$ — сумма квадратов невязок (m.2.1.3).

Докажем, что сумма квадратов невязок при векторе параметров, найденном МНК, не больше суммы квадратов шума. Подставим (m.2.1.3), затем (m.2.3.7):

$$
\sum_{i=1}^{m} \varepsilon_i(\hat{\bar{x}})^2 = \Phi(\hat{\bar{x}}) = \min_{\bar{x}} \Phi(\bar{x}) \le \Phi(\bar{x}^{*}) = \sum_{i=1}^{m} \varepsilon_i(\bar{x}^{*})^2 = \sum_{i=1}^{m} w_i^2
$$

Разделим обе части на $m$ и извлечём корень, чтобы правая часть приняла вид (m.2.3.8):

$$
\boxed{\sqrt{\frac{1}{m}\sum_{i=1}^{m} \varepsilon_i(\hat{\bar{x}})^2} \le \sqrt{\frac{1}{m}\sum_{i=1}^{m} w_i^2}}
$$

Вывод в линейном приближении. Вблизи $\bar{x}^{*}$ невязки линейны по параметрам (шум много меньше $L$):

$$
\bar{\varepsilon}(\bar{x}) \approx \bar{w} + J\,(\bar{x} - \bar{x}^{*})
$$

где $\bar{\varepsilon}(\bar{x})$ — вектор из $m$ невязок;
&emsp;&emsp;$\bar{w}$ — вектор из $m$ значений шума;
&emsp;&emsp;$J$ — матрица Якоби (m.2.2.1).

Минимум суммы квадратов этого выражения:

$$
\boxed{\begin{aligned}
\hat{\bar{x}} - \bar{x}^{*} &= -(J^T J)^{-1} J^T \bar{w}\\
\bar{\varepsilon}(\hat{\bar{x}}) &= (I - H)\,\bar{w}, \qquad H = J\,(J^T J)^{-1} J^T
\end{aligned}}
\tag{m.2.3.9}
$$

где $I$ — единичная матрица $m \times m$;
&emsp;&emsp;$H$ — матрица $m \times m$.

$(J^T J)^{-1}$ существует при $\operatorname{rank} J = n$ (m.2.2.19).

В какой мере шум переходит в ошибку найденного центра шарика, зависит от набора ориентаций через матрицу $J$ в первом равенстве (m.2.3.9). Для конкретного набора касаний она вычисляется по матрице $J$ и СКО шума (раздел 4).

Три свойства, каждое проверяется подстановкой:

| <nobr>Свойство</nobr>                                                                                             | <nobr>Почему</nobr>                                                                                                                                                                                                                                                                      |
| ---------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| $(I - H)^T = I - H$ и $(I - H)^2 = I - H$                                                            | $H H = J (J^T J)^{-1} J^T J (J^T J)^{-1} J^T = H$                                                                                                                                                                                                                           |
| $\operatorname{tr} H = n$                                                                            | след $\operatorname{tr}$ (сумма диагональных элементов) не меняется при перестановке сомножителей: $\operatorname{tr}\left(J (J^T J)^{-1} J^T\right) = \operatorname{tr}\left((J^T J)^{-1} J^T J\right) = n$, так как $(J^T J)^{-1} J^T J$ — единичная матрица $n \times n$ |
| для любой матрицы $M$ размером $m \times m$: $E[\bar{w}^T M \bar{w}] = \sigma^2 \operatorname{tr} M$ | $\bar{w}^T M \bar{w} = \sum_{i} \sum_{i'} M_{ii'} w_i w_{i'}$, где $i, i' = 1 \ldots m$ — номера касаний; при независимом шуме со средним 0 $E[w_i w_{i'}] = \sigma^2$ при $i = i'$ и $0$ при $i \ne i'$                                                                    |

Отсюда математическое ожидание суммы квадратов невязок после оценивания:

$$
E\left[\sum_{i=1}^{m} \varepsilon_i(\hat{\bar{x}})^2\right]
= E\left[\bar{w}^T (I - H)^T (I - H)\,\bar{w}\right]
= E\left[\bar{w}^T (I - H)\,\bar{w}\right]
= \sigma^2\,(m - n)
$$

$$
\boxed{E\left[\sum_{i=1}^{m} \varepsilon_i(\hat{\bar{x}})^2\right] = \sigma^2\,(m - n)}
\tag{m.2.3.10}
$$

Оценивание в среднем убирает из суммы квадратов ровно $n\sigma^2$ — по $\sigma^2$ на каждый искомый параметр. Поэтому СКО невязок после оценивания

$$
\boxed{s = \sqrt{\frac{1}{m - n}\sum_{i=1}^{m} \varepsilon_i(\hat{\bar{x}})^2}}
\tag{m.2.3.11}
$$

удовлетворяет $E[s^2] = \sigma^2$: $s$ — оценка СКО шума одного касания по невязкам. Разность $m - n$ называется числом степеней свободы; в случае 1 при условных 37 касаниях она равна $37 - 6 = 31$.

Пример раздела 2.3.2 ($n = 1$). Подставим (m.2.3.1) в (m.2.2.1), затем в $H$ из (m.2.3.9):

$$
J = \frac{\partial}{\partial y}\begin{bmatrix} y_1 - y \\ \vdots \\ y_m - y \end{bmatrix} = \begin{bmatrix} -1 \\ \vdots \\ -1 \end{bmatrix},
\qquad
H = J\,(J^T J)^{-1} J^T = \frac{1}{m}\begin{bmatrix} 1 & \cdots & 1 \\ \vdots & \ddots & \vdots \\ 1 & \cdots & 1 \end{bmatrix},
\qquad
\operatorname{tr} H = m \cdot \frac{1}{m} = 1 = n
$$

где $\operatorname{tr} H$ — след матрицы $H$.

Формулы (m.2.3.10) и (m.2.3.11) при $n = 1$:

$$
E\left[\sum_{i=1}^{m}(y_i - \bar{y})^2\right] = \sigma^2\,(m - 1),
\qquad
s = \sqrt{\frac{1}{m - 1}\sum_{i=1}^{m}(y_i - \bar{y})^2}
$$

где $s$ — СКО невязок после оценивания, мм;
&emsp;&emsp;$\sigma$ — истинное СКО шума одного измерения, мм.

#### 2.3.4. Крайний случай m = n

При $m = n$ матрица $J$ квадратная и обратима, поэтому в (m.2.3.9) $H = J J^{-1} (J^T)^{-1} J^T = I$ и $\bar{\varepsilon}(\hat{\bar{x}}) = (I - I)\,\bar{w} = 0$ при любом шуме: $n$ параметров принимают такие значения, что все $n$ уравнений касания выполняются точно. Шум целиком уходит в ошибку параметров, а в невязках не остаётся ничего.

| <nobr>Пример</nobr>                                         | <nobr>Что</nobr> <nobr>происходит</nobr>                                                    | <nobr>Делитель</nobr> $m$                              | <nobr>Делитель</nobr> $m - n$                                                 |
| ---------------------------------------------- | ----------------------------------------------------------------- | ----------------------------------------- | ---------------------------------------------------------------- |
| 1 параметр, 1 касание ($m = n = 1$)            | $\hat{y} = y_1$, невязка $\varepsilon_1 = 0$ при любом шуме $w_1$ | $s = 0$: ложно показывает отсутствие шума | $0/0$: $s$ не определено — одно касание ничего не говорит о шуме |
| 6 параметров случая 1, 6 касаний ($m = n = 6$) | все 6 невязок равны 0 при любом шуме (в линейном приближении)     | $s = 0$, ложно                            | $0/0$, $s$ не определено                                         |

#### 2.3.5. Ожидаемый шум и проверки невязок

Ожидаемое СКО шума $\sigma_0$ — значение $\sigma$, которое следует из известных случайных источников шума:

| <nobr>Источник</nobr> <nobr>шума</nobr>                                                                                                    | <nobr>Распределение</nobr>                                    | <nobr>СКО</nobr>                       |
| ---------------------------------------------------------------------------------------------------------------- | ------------------------------------------------ | ------------------------- |
| момент опроса входа срабатывания (раздел 5): после срабатывания TCP движется до обнаружения ещё от 0 до $T_{in}$ | равномерное, перебег шарика от 0 до $v_s T_{in}$ | $v_s T_{in} / \sqrt{12}$  |
| циклы промежуточных устройств на пути сигнала срабатывания (раздел 5)                                            | зависит от устройств                             | оценивается для установки |
| разрешение и шум датчиков углов сочленений, разрешение передаваемых углов                                        | зависит от системы управления                    | оценивается для установки |

СКО независимых источников складываются как $\sigma_0 = \sqrt{\sigma_1^2 + \sigma_2^2 + \ldots}$, где $\sigma_1, \sigma_2, \ldots$ — СКО отдельных источников. Пример с условными значениями $T_{in}$ = 4 мс, $v_s$ = 1 мм/с: СКО от опроса входа $0{,}004 / \sqrt{12} = 0{,}0012$ мм; если другие источники не оценены, известна только нижняя граница $\sigma_0 \ge 0{,}0012$ мм.

Термины проверок:

| <nobr>Термин</nobr>                                             | <nobr>Определение</nobr>                                                                                                                             |
| -------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------- |
| нормальный шум                                     | шум с нормальным (гауссовым) распределением                                                                                             |
| распределение $\chi^2$ с $m - n$ степенями свободы | распределение суммы квадратов $m - n$ независимых нормально распределённых величин со средним 0 и СКО 1                                 |
| квантиль 0,99                                      | значение, которое случайная величина превышает с вероятностью 1 %; для $\chi^2$ — табличное: 52,19 при 31 степени свободы, 50,89 при 30 |

Две проверки невязок:

| <nobr>Проверка</nobr>                                  | <nobr>Что</nobr> <nobr>сравнивается</nobr>                    | <nobr>Когда</nobr> <nobr>проверка</nobr> <nobr>не</nobr> <nobr>пройдена</nobr>            | <nobr>Откуда</nobr> <nobr>граница</nobr>                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 | <nobr>Что</nobr> <nobr>делать</nobr>                                                                                                              |
| ----------------------------------------- | ----------------------------------- | ------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| 1. Разброс невязок против известного шума | $s$ и $\sigma_0$                    | $s / \sigma_0 > 1{,}30$               | $s$ вычислено по случайным невязкам и само случайно: даже если $\sigma = \sigma_0$, $s \ne \sigma_0$. При нормальном шуме $(m - n)\, s^2 / \sigma^2 = \bar{w}^T (I - H)\, \bar{w} / \sigma^2$ распределено по закону $\chi^2$ с $m - n$ степенями свободы: матрица $I - H$ симметрична и $(I - H)^2 = I - H$, поэтому её собственные значения — только 0 и 1, а их сумма (след) равна $m - n$; после поворота осей к собственным векторам это сумма квадратов $m - n$ независимых нормальных величин со СКО 1; значение 52,19 оно превышает с вероятностью 1 % (квантиль 0,99 при 31 степени свободы). Граница: $\sqrt{52{,}19 / 31} = 1{,}30$ | искать неучтённый источник шума или неучтённое явление (случаи 2 и 3)                                                   |
| 2. Выброс                                 | $\lvert \varepsilon_i \rvert$ и $s$ | $\lvert \varepsilon_i \rvert / s > 3$ | при нормальном шуме $\lvert \varepsilon_i \rvert > 3$ СКО с вероятностью 0,27 %; на условных 37 касаниях это в среднем $37 \cdot 0{,}0027 = 0{,}1$ ложного выброса на калибровку                                                                                                                                                                                                                                                                                                                                                                                                                                                               | касание $i$ исключить и повторить; возможные причины — пыль на сфере, ложное срабатывание щупа, неверно записанные углы |

Ограничение проверки 1: шум опроса входа распределён равномерно, а не нормально, поэтому граница 1,30 приближённая.

Пример с условными значениями: $s = 0{,}004$ мм, $\varepsilon_5 = -0{,}010$ мм.

| <nobr>Проверка</nobr> | <nobr>Вычисление</nobr>                  | <nobr>Вывод</nobr>                                                                                                                        |
| -------- | --------------------------- | ---------------------------------------------------------------------------------------------------------------------------- |
| 1        | 0,004 / 0,0012 = 3,3 > 1,30 | модель случая 1 с известным шумом невязки не объясняет: либо неоценённые источники шума больше, либо есть неучтённое явление |
| 2        | 0,010 / 0,004 = 2,5 < 3     | касание 5 не выброс                                                                                                          |

Отличить шум от систематической части по невязкам одного касания нельзя. Это возможно, если у нескольких касаний есть общая причина систематической части; такая проверка вводится в случае 3 (раздел 2.5).

### 2.4. Случай 2: постоянный перебег шарика после касания

Согласно (m.1.1), к моменту обнаружения TCP проходит после касания путь $\delta_p + v_s\,t_{lat}$ вдоль направления подхода, то есть к центру сферы. Поэтому зарегистрированный центр шарика находится от центра сферы на расстоянии

$$
\boxed{\rho = L - \delta_p - v_s\,t_{lat}}
$$

Пример с условными значениями раздела 1: $\rho = 14 - 0{,}008 - 0{,}015 = 13{,}977$ мм.

Допущение случая: ход до срабатывания и средняя задержка сигнала одинаковы у всех касаний; скорость поиска задана одинаковой. Случайная часть задержки, как в случае 1, — шум.

Первые три элемента строки (m.2.2.11) через наклон щупа $\tau_i$ и поворот щупа вокруг оси стилуса $\psi_i$ при касании $i$:

$$
\begin{aligned}
{}^{F}\bar{e}_{sr}(\bar{q}_i,\bar{x}) &= {}^{B}R_F(\bar{q}_i)^T\,{}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x}) && \text{(m.2.2.10), точно}\\
{}^{B}R_F(\bar{q}_i) &\approx {}^{B}R_P(\lambda_i,\mu_i,\tau_i,\psi_i) && \text{допущение 1}\\
{}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x}) &\approx {}^{B}\bar{e}_n(\lambda_i,\mu_i) && \text{допущение 2}\\
{}^{F}\bar{e}_{sr}(\bar{q}_i,\bar{x}) &\approx {}^{B}R_P(\lambda_i,\mu_i,\tau_i,\psi_i)^T\,{}^{B}\bar{e}_n(\lambda_i,\mu_i) = -\,{}^{P}\bar{e}_a(\tau_i,\psi_i) && \text{(2.2.2), (2.2.4)}
\end{aligned}
\tag{m.2.4.1}
$$

где ${}^{F}\bar{e}_{sr}(\bar{q}_i,\bar{x}),\ {}^{B}\bar{e}_{sr}(\bar{q}_i,\bar{x})$ — орт вектора из центра сферы $s$ в рассчитанный центр шарика $r$ при касании $i$, выраженный в {F} и в {B};
&emsp;&emsp;${}^{B}R_F(\bar{q}_i)$ — поворот {F} относительно {B} по измеренным углам сочленений;
&emsp;&emsp;${}^{B}R_P(\lambda_i,\mu_i,\tau_i,\psi_i)$ — поворот {P} относительно {B} по плану касания $i$ (2.2.3);
&emsp;&emsp;$\lambda_i,\ \mu_i$ — широта и долгота запланированной точки касания $i$, градусы;
&emsp;&emsp;$\tau_i$ — наклон щупа при касании $i$: угол между осью стилуса и направлением подхода, градусы;
&emsp;&emsp;$\psi_i$ — поворот щупа вокруг оси стилуса при касании $i$, градусы;
&emsp;&emsp;${}^{B}\bar{e}_n(\lambda_i,\mu_i)$ — орт внешней нормали сферы в запланированной точке касания $i$, в {B};
&emsp;&emsp;${}^{P}\bar{e}_a(\tau_i,\psi_i)$ — орт направления подхода при касании $i$, в {P}.

| <nobr>Строка</nobr> <nobr>цепочки</nobr> | <nobr>Основание</nobr>                                                                                                                                                                                                          | <nobr>Погрешность</nobr>                                                                                                    |
| -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------------- |
| 1              | формула (m.2.2.10)                                                                                                                                                                                                   | нет                                                                                                            |
| 2, допущение 1 | фланец повёрнут так, как задано планом касания; оси {P} параллельны осям {F}, поэтому поворот {P} и поворот {F} относительно {B} — одна матрица                                                                    | определяется точностью ориентации робота; не оценена                                                           |
| 3, допущение 2 | касание произошло в запланированной точке, и параметры близки к истинным; у двух касающихся сфер центры и точка касания лежат на одной прямой, поэтому вектор от центра сферы к центру шарика направлен по нормали | смещение центра сферы на 0,1 мм поперёк нормали поворачивает орт примерно на $0{,}1 / 14$ рад $\approx 0{,}4°$ |
| 4              | подстановка строк 2 и 3 в строку 1; затем формулы (2.2.2) и (2.2.4)                                                                                                                                                  | наследует допущения 1 и 2; формула (2.2.4) численно не проверена                                                 |

Подставим строку 4 цепочки (m.2.4.1) в (m.2.2.11), затем третий элемент (2.2.4):

$$
\boxed{\frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{F}p_{r,z}} = J_{i3} \approx -\cos\tau_i}
\tag{m.2.4.2}
$$

где $\tau_i$ — наклон щупа при касании $i$.

Этот результат наследует допущения 1 и 2.

Что будет, если оставить в уравнениях касания известное $L$? Подставим (m.2.4.2) в изменение невязки касания $i$ при сдвиге центра шарика вдоль оси стилуса:

$$
\Delta\varepsilon_i \approx \frac{\partial \varepsilon_i(\bar{x})}{\partial\,{}^{F}p_{r,z}}\,\Delta{}^{F}p_{r,z} \approx -\cos\tau_i\,\Delta{}^{F}p_{r,z}
$$

где $\Delta{}^{F}p_{r,z}$ — сдвиг центра шарика вдоль оси стилуса (от фланца), элемент 3 вектора (m.2.1.1), мм; $\left|\Delta{}^{F}p_{r,z}\right| \ll L$;
&emsp;&emsp;$\Delta\varepsilon_i$ — изменение невязки касания $i$, мм.

Отсюда:

| <nobr>Наклоны</nobr> <nobr>щупа</nobr> <nobr>при</nobr> <nobr>касаниях</nobr>                        | <nobr>Результат</nobr> <nobr>оценивания</nobr> с <nobr>известным</nobr> $L$                                                                                                        |
| ------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------- |
| все $\tau = 0$                                   | сдвиг на 0,023 мм к фланцу делает все невязки нулевыми; центр шарика найден с ошибкой 0,023 мм вдоль стилуса, и невязки этого не показывают |
| $\tau = 0 \ldots 55°$ ($\cos\tau$ от 1 до 0,574) | один сдвиг не обнуляет все невязки; часть ошибки уходит в центр шарика, часть остаётся в невязках; величины не вычислялись                  |

Предложение: оценивать расстояние касания как седьмой искомый параметр $\rho$ вместо известного $L$:

$$
\bar{x} = \begin{bmatrix} {}^{F}p_{r,x} & {}^{F}p_{r,y} & {}^{F}p_{r,z} & {}^{B}p_{s,x} & {}^{B}p_{s,y} & {}^{B}p_{s,z} & \rho \end{bmatrix}^{T},
\qquad
\varepsilon_i(\bar{x}) = \left\| {}^{B}\bar{p}_F(\bar{q}_i) + {}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{p}_r - {}^{B}\bar{p}_s \right\| - \rho
\tag{m.2.4.3}
$$

Оценивание по МНК находит одно число $\rho$, но не слагаемые, из которых оно состоит. Если любое из этих слагаемых отличается от предположенного, оценивание находит другое $\rho$, а центр шарика и центр сферы не меняются. Поэтому слагаемые по отдельности неидентифицируемы: идентифицируема только их комбинация $\rho$. Для калибровки знать их не нужно, но и найти их по отдельности она не позволяет. Пример: истинный радиус сферы 12,497 мм вместо 12,5 мм даёт $\rho$ = 13,974 мм вместо 13,977 мм при том же центре шарика.

| <nobr>Величина</nobr>                                         | <nobr>Значение</nobr> в <nobr>условном</nobr> <nobr>примере</nobr>                                         | <nobr>Входит</nobr> в $\rho$ и <nobr>по</nobr> <nobr>отдельности</nobr> <nobr>неидентифицируема?</nobr> |
| ------------------------------------------------ | ------------------------------------------------------------------- | --------------------------------------------------- |
| радиус шарика                                    | 1,5 мм                                                              | да                                                  |
| радиус сферы                                     | 12,5 мм                                                             | да                                                  |
| ход до срабатывания, одинаковый у всех касаний   | неизвестен; в примере 0,008 мм                                      | да                                                  |
| перебег шарика за время средней задержки сигнала | 0,015–0,020 мм при скорости поиска 1 мм/с (раздел 5)                | да, если скорость поиска одинакова у всех касаний   |
| случайная часть задержки сигнала                 | перебег шарика 0…0,004 мм, СКО 0,0012 мм при скорости поиска 1 мм/с | нет: это шум, он остаётся в невязках                |

Что меняется по сравнению со случаем 1:

| <nobr>Что</nobr>                                       | <nobr>Случай</nobr> 1                       | <nobr>Случай</nobr> 2                                                                                              |
| ----------------------------------------- | ------------------------------ | ----------------------------------------------------------------------------------------------------- |
| число искомых параметров $n$              | 6                              | 7                                                                                                     |
| число степеней свободы $m - n$            | 31                             | 30                                                                                                    |
| граница проверки 1 (раздел 2.3.5)         | $\sqrt{52{,}19 / 31} = 1{,}30$ | $\sqrt{50{,}89 / 30} = 1{,}30$ (50,89 — квантиль 0,99 распределения $\chi^2$ при 30 степенях свободы) |
| условие идентифицируемости 1 (раздел 2.2) | $m \ge 6$                      | $m \ge 7$                                                                                             |
| условие идентифицируемости 4              | —                              | наклон щупа $\tau \ne 0$ у части касаний                                                              |

Условие 4 в терминах матрицы Якоби (раздел 2.2). С седьмым параметром у $J$ появляется седьмой столбец — производные (m.2.4.3) по $\rho$. Третий столбец — производная по координате z центра шарика в {F} — не изменился по сравнению со случаем 1: его элементы даёт (m.2.4.2). Седьмой столбец:

$$
J_{i7} = \frac{\partial \varepsilon_i(\bar{x})}{\partial \rho} = -1
$$

| <nobr>Наклоны</nobr> <nobr>щупа</nobr> <nobr>при</nobr> <nobr>касаниях</nobr> | <nobr>Столбцы</nobr> 3 и 7                                            | $\Delta\bar{x}$ с $J\,\Delta\bar{x} = \bar{0}$                                                                                                                                 | <nobr>Вывод</nobr>                                                                                              |
| ------------------------- | -------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------- |
| все $\tau = 0$            | оба столбца состоят из −1 и совпадают                    | $\Delta\bar{x} = (0, 0, 1, 0, 0, 0, -1)^T$: сдвиг центра шарика на 1 мм вдоль стилуса и уменьшение $\rho$ на 1 мм. Проверка: $J_{i3} \cdot 1 + J_{i7} \cdot (-1) = -1 + 1 = 0$ | составляющая центра шарика вдоль стилуса и $\rho$ неидентифицируемы: $\operatorname{rank} J \le 6$ |
| $\tau = 0 \ldots 55°$     | столбец 3 — от −1 до −0,574, не пропорционален столбцу 7 | эта зависимость исчезает                                                                                                                                                       | составляющая вдоль стилуса и $\rho$ разделяются; полная проверка — $\operatorname{rank} J = 7$     |

Эффективный радиус шарика $r_{eff}$ — радиус шарика за вычетом пути щупа от касания до обнаружения касания; он нужен, чтобы по зарегистрированному центру шарика вычислять точку касания детали, у которой нет эталонной сферы и расстояние касания оценить нельзя. Получим $r_{eff}$ из найденного расстояния касания и вектор ошибки точки касания, вычисленной с этим радиусом. Точка касания — точка поверхности детали, в которой её касается шарик.

Подставим в $\rho = L - \delta_p - v_s\,t_{lat}$ значение $L = r_b + r_s$ и вычтем радиус сферы:

$$
r_{eff} = \hat{\rho} - r_s = r_b - \delta_{p0} - v_s\,t_{lat}
\tag{m.2.4.4}
$$

где $r_{eff}$ — эффективный радиус шарика, мм;
&emsp;&emsp;$\hat{\rho}$ — расстояние касания, найденное МНК (m.2.4.3), мм;
&emsp;&emsp;$r_s$ — радиус эталонной сферы, мм;
&emsp;&emsp;$r_b$ — радиус шарика, мм;
&emsp;&emsp;$\delta_{p0}$ — ход до срабатывания, одинаковый у всех касаний калибровки (допущение случая 2), мм;
&emsp;&emsp;$v_s$ — скорость поиска, мм/с;
&emsp;&emsp;$t_{lat}$ — средняя задержка сигнала, с.

Центр шарика здесь оценивается вместе с $\hat{\rho}$, поэтому ориентация щупа при касаниях может меняться. Если вместо этого центр шарика считается известным и по зарегистрированным центрам шарика аппроксимируется только сфера, ориентация щупа должна быть одной и той же при всех касаниях: тогда ошибка принятого центра шарика сдвигает все зарегистрированные центры шарика на один и тот же вектор, этот сдвиг уходит в центр аппроксимирующей сферы и не меняет её радиус (условие 2 раздела 2.2.5); при меняющейся ориентации сдвиги разные и входят в радиус аппроксимирующей сферы.

При измерении детали точку касания вычисляют по зарегистрированному центру шарика, смещая его на $r_{eff}$ против внешней нормали поверхности детали:

$$
{}^{B}\bar{p}_{surf} = {}^{B}\bar{p}_{det} - r_{eff}\,{}^{B}\bar{e}_{n.surf}
\tag{m.2.4.5}
$$

где ${}^{B}\bar{p}_{surf}$ — вычисленная точка касания детали, мм;
&emsp;&emsp;${}^{B}\bar{p}_{det}$ — зарегистрированный центр шарика при касании детали: рассчитанный центр шарика (раздел 2.1) по углам сочленений в момент обнаружения касания, мм;
&emsp;&emsp;${}^{B}\bar{e}_{n.surf}$ — орт внешней нормали поверхности детали в точке касания;
&emsp;&emsp;$r_{eff}$ — эффективный радиус шарика (m.2.4.4), мм.

Центр шарика в момент касания отстоит от зарегистрированного центра шарика на путь щупа от касания до обнаружения касания (m.1.1) вдоль внешней нормали, а фактическая точка касания — от центра шарика в момент касания на $r_b$ против нормали:

$$
{}^{B}\bar{p}_{c} = {}^{B}\bar{p}_{det} + \left(\delta_p + v_s\,t_{lat}\right){}^{B}\bar{e}_{n.surf},
\qquad
{}^{B}\bar{p}_{act} = {}^{B}\bar{p}_{c} - r_b\,{}^{B}\bar{e}_{n.surf}
$$

где ${}^{B}\bar{p}_{c}$ — центр шарика в момент касания детали, мм;
&emsp;&emsp;${}^{B}\bar{p}_{act}$ — фактическая точка касания детали, мм;
&emsp;&emsp;$\delta_p$ — ход до срабатывания при касании детали, мм;
&emsp;&emsp;$v_s$ — скорость поиска, та же, что при калибровке, мм/с;
&emsp;&emsp;$t_{lat}$ — средняя задержка сигнала, с;
&emsp;&emsp;$r_b$ — радиус шарика, мм.

Вектор ошибки вычисленной точки касания — разность вычисленной и фактической точек касания при точных центре шарика и позе фланца (их ошибки — разделы 3, 4). Подставим (m.2.4.5) и выражения для ${}^{B}\bar{p}_{act}$ и ${}^{B}\bar{p}_{c}$:

$$
{}^{B}\bar{p}_{surf} - {}^{B}\bar{p}_{act} = {}^{B}\bar{p}_{det} - r_{eff}\,{}^{B}\bar{e}_{n.surf} - {}^{B}\bar{p}_{det} - \left(\delta_p + v_s\,t_{lat}\right){}^{B}\bar{e}_{n.surf} + r_b\,{}^{B}\bar{e}_{n.surf} = \left(r_b - r_{eff} - \delta_p - v_s\,t_{lat}\right){}^{B}\bar{e}_{n.surf}
$$

Подставим $r_b - r_{eff} = \delta_{p0} + v_s\,t_{lat}$ из (m.2.4.4):

$$
\boxed{{}^{B}\bar{p}_{surf} - {}^{B}\bar{p}_{act} = \left(\delta_{p0} - \delta_p\right){}^{B}\bar{e}_{n.surf}}
$$

где $\delta_{p0}$ — ход до срабатывания при касаниях калибровки, мм;
&emsp;&emsp;$\delta_p$ — ход до срабатывания при касании детали, мм.

Следовательно, при той же скорости поиска эффективный радиус шарика $r_{eff}$ компенсирует ход до срабатывания при калибровке $\delta_{p0}$ и перебег за среднюю задержку сигнала $v_s\,t_{lat}$; вектор ошибки вычисленной точки касания направлен по нормали к поверхности детали, и его проекция на внешнюю нормаль равна разности хода до срабатывания при калибровке $\delta_{p0}$ и хода до срабатывания при касании детали $\delta_p$, то есть $\delta_{p0} - \delta_p$.

### 2.5. Случай 3: ход до срабатывания зависит от направления нормальной силы в точке касания

Если ход до срабатывания зависит от направления нормальной силы в точке касания относительно корпуса щупа, как у кинематических резистивных щупов Renishaw (L1, раздел 3.3.1), то он зависит от наклона щупа и поворота щупа вокруг оси стилуса (раздел 2.2 теории):

$$
\rho_i = L - \delta_p(\tau_i,\psi_i) - v_s\,t_{lat}
$$

где $\rho_i$ — расстояние касания $i$;
&emsp;&emsp;$\delta_p(\tau_i,\psi_i)$ — ход до срабатывания при касании $i$, у которого наклон щупа $\tau_i$ и поворот щупа вокруг оси стилуса $\psi_i$.

Оценивание по МНК по-прежнему находит одно $\rho$. Чтобы понять, что оно делает с различиями между касаниями, ход до срабатывания раскладывается на четыре слагаемых:

$$
\delta_p(\tau_i,\psi_i) = \delta_{p0} + \left({}^{P}\bar{u}\right)^{T}\,{}^{P}\bar{e}_a(\tau_i,\psi_i) + \left({}^{B}\bar{f}\right)^{T}\,{}^{B}\bar{e}_n(\lambda_i,\mu_i) + \eta_i
\tag{m.2.5.1}
$$

где $\delta_{p0}$ — постоянная часть, мм;
&emsp;&emsp;$\bar{u}$ — постоянный вектор, выраженный в {P}, мм;
&emsp;&emsp;${}^{P}\bar{e}_a(\tau_i,\psi_i)$ — орт направления подхода при касании $i$ (2.2.4);
&emsp;&emsp;$\bar{f}$ — постоянный вектор, выраженный в {B}, мм;
&emsp;&emsp;${}^{B}\bar{e}_n(\lambda_i,\mu_i)$ — орт внешней нормали сферы при касании $i$ (2.2.1);
&emsp;&emsp;$\eta_i$ — всё остальное, мм.

Значения $\delta_{p0}$, $\bar{u}$, $\bar{f}$ выбираются по методу наименьших квадратов по $m$ касаниям, поэтому $\eta_i$ — то, что эти три слагаемых описать не могут.

Почему выбраны именно эти слагаемые: так меняется расстояние касания при малом сдвиге параметра (приближение линейное, как в случае 2).

| <nobr>Сдвиг</nobr> <nobr>параметра</nobr>                                 | <nobr>Изменение</nobr> <nobr>невязки</nobr> <nobr>касания</nobr> $i$                                                                                      |
| ----------------------------------------------- | ------------------------------------------------------------------------------------------------------------------ |
| расстояние касания изменено на $\Delta\rho$ мм  | одинаково у всех касаний: $-\Delta\rho$ (невязка (m.2.4.3))                                                          |
| центр шарика сдвинут на постоянный вектор в {F} | минус скалярное произведение этого вектора на направление подхода в {P} касания $i$ (оси {P} параллельны осям {F}) |
| центр сферы сдвинут на постоянный вектор в {B}  | минус скалярное произведение этого вектора на нормаль касания $i$                                                  |

Каждое из первых трёх слагаемых хода до срабатывания совпадает по форме с одной строкой этой таблицы, поэтому оценивание по МНК воспроизводит его сдвигом соответствующего параметра:

| <nobr>Слагаемое</nobr> <nobr>хода</nobr> <nobr>до</nobr> <nobr>срабатывания</nobr>             | <nobr>Чем</nobr> <nobr>оценивание</nobr> <nobr>его</nobr> <nobr>воспроизводит</nobr>          | <nobr>Последствие</nobr> <nobr>для</nobr> <nobr>калибровки</nobr>                                                                                                                                             |
| ------------------------------------------ | ----------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1-е, постоянное                            | изменением расстояния касания             | центр шарика не меняется; постоянная часть $\delta_{p0}$ входит в эффективный радиус шарика вместо одинакового хода до срабатывания в (m.2.4.4)                          |
| 2-е, линейное по направлению подхода в {P} | сдвигом центра шарика на вектор $\bar{u}$ | найденный центр шарика смещён на $\bar{u}$ от истинного: это «эффективный» центр шарика                                                                                |
| 3-е, линейное по нормали в {B}             | сдвигом центра сферы на вектор $\bar{f}$  | центр сферы — вспомогательный параметр; на центр шарика не влияет                                                                                                      |
| 4-е, $\eta_i$                              | ничем                                     | остаётся в невязках как систематическая часть: повторное касание с той же парой $(\tau, \psi)$ даёт ту же $\eta_i$; найденные параметры в первом приближении не меняет |

Выводы:

- Ошибку найденного центра шарика даёт только 2-е слагаемое (вектор $\bar{u}$). Это слагаемое — наилучшее линейное приближение хода до срабатывания по использованным направлениям, поэтому другой набор касаний дал бы другое $\bar{u}$.
- Для последующих измерений поверхностей смещение на $\bar{u}$, по предварительному выводу, не ошибка (доказательство не приведено): «эффективный» центр шарика учитывает и линейную часть хода до срабатывания. Численно не проверено.
- 4-е слагаемое делает невязку касания $i$ равной $\eta_i$ плюс шум: рассчитанные центры шарика отстоят от найденной сферы на $\eta_i$.
- Величина $\eta_i$ зависит от щупа; у щупа Renishaw TP6 со стилусом 50 мм размах хода до срабатывания по направлениям — 3,28 мкм (L1, раздел 3.3.1).
- Таблица и выводы получены линеаризацией в этом документе; численно не проверены. Они уточняют прежнюю формулировку «отклонение от среднего искажает центр шарика»: искажает только его линейная по направлению подхода часть.

#### Проверка 3: систематическая зависимость невязок от пары $(\tau, \psi)$

У касаний одной пары $(\tau, \psi)$ (раздел 2.2 теории) слагаемое $\eta_i$ одинаково, а шум разный. Поэтому среднее невязок пары оценивает $\eta_i$ этой пары:

$$
\langle \varepsilon \rangle_k = \frac{1}{m_k}\sum_{i \in k} \varepsilon_i
$$

где $k$ — номер пары;
&emsp;&emsp;$\langle \varepsilon \rangle_k$ — среднее невязок касаний пары $k$, мм;
&emsp;&emsp;$m_k$ — число касаний пары $k$;
&emsp;&emsp;$i \in k$ — касания пары $k$.

| <nobr>Проверка</nobr>                               | <nobr>Когда</nobr> <nobr>проверка</nobr> <nobr>не</nobr> <nobr>пройдена</nobr>                                       | <nobr>Откуда</nobr> <nobr>граница</nobr>                                                                                                                  | <nobr>Вывод</nobr>                                                                          |
| -------------------------------------- | ---------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------ |
| 3. Систематическая зависимость от пары | $\lvert \langle \varepsilon \rangle_k \rvert > 3 s / \sqrt{m_k}$ | среднее $m_k$ значений шума со СКО $s$ имеет СКО $s / \sqrt{m_k}$ (стандартная ошибка среднего); множитель 3 — как в проверке 2 | у пары $k$ своё $\eta_i$: ход до срабатывания этой пары отличается от среднего |

Проверка 3 возможна только для пар с несколькими касаниями. У пары с одним касанием среднее равно единственной невязке, и отличить $\eta_i$ от шума нельзя.

Продолжение примера раздела 2.3.5 с условными значениями, теперь для модели случая 3: $s = 0{,}004$ мм, $\varepsilon_5 = -0{,}010$ мм; касание 5 входит в пару 1 ($m_1 = 14$), $\langle \varepsilon \rangle_1 = -0{,}006$ мм.

| <nobr>Проверка</nobr>                   | <nobr>Вычисление</nobr>                                                                                                                   | <nobr>Вывод</nobr>                                                                                  |
| -------------------------- | ---------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------- |
| 3, пара 1                  | $3 \cdot 0{,}004 / \sqrt{14} = 0{,}0032$ мм; $0{,}006 > 0{,}0032$                                                            | у касаний с $\tau = 0$ ход до срабатывания отличается от среднего по всем касаниям     |
| разложение $\varepsilon_5$ | $\varepsilon_5 = \langle \varepsilon \rangle_1 + (\varepsilon_5 - \langle \varepsilon \rangle_1) = -0{,}006 + (-0{,}004)$ мм | систематическая часть пары 1: −0,006 мм; шум касания 5: −0,004 мм, то есть $1 \cdot s$ |

### 2.6. Сводка случаев

| <nobr>Величина</nobr>                                             | <nobr>Случай</nobr> 1            | <nobr>Случай</nobr> 2                                          | <nobr>Случай</nobr> 3                                                            |
| ---------------------------------------------------- | ------------------- | ------------------------------------------------- | ------------------------------------------------------------------- |
| центр шарика в {F}                                   | искомый             | искомый                                           | искомый; найден со смещением на линейную часть хода до срабатывания |
| центр сферы в {B}                                    | искомый             | искомый                                           | искомый                                                             |
| расстояние касания                                   | $L$, известно       | $\rho$, искомый                                   | $\rho$, искомый                                                     |
| радиусы шарика и сферы                               | входят в $L$        | входят в $\rho$, по отдельности неидентифицируемы | входят в $\rho$, по отдельности неидентифицируемы                   |
| постоянная часть хода до срабатывания                | не учитывается      | входит в $\rho$                                   | входит в $\rho$                                                     |
| перебег шарика за время средней задержки сигнала     | не учитывается      | входит в $\rho$                                   | входит в $\rho$                                                     |
| случайная часть задержки сигнала                     | шум, в невязках     | шум, в невязках                                   | шум, в невязках                                                     |
| различия хода до срабатывания между касаниями        | не учитываются      | не учитываются                                    | линейные части — в центрах шарика и сферы, $\eta_i$ — в невязках    |
| число искомых параметров $n$                         | 6                   | 7                                                 | 7                                                                   |
| число степеней свободы $m - n$ при условных $m = 37$ | 31                  | 30                                                | 30                                                                  |
| условия идентифицируемости (разделы 2.2, 2.4)        | 1–3                 | 1–4                                               | 1–4                                                                 |
| проверки невязок                                     | 1, 2 (раздел 2.3.5) | 1, 2                                              | 1, 2, 3 (раздел 2.5)                                                |

## 3. Ошибки робота, которые искажают найденный центр шарика

Докажем, что постоянные ошибки со стороны основания робота уходят в центр сферы, постоянные ошибки со стороны фланца — в центр шарика, и ни те ни другие не дают невязок и не искажают последующие измерения тем же роботом.

Термины раздела:

- {B'} — система, в которой номинальная кинематическая цепь робота фактически начинается: основание с ошибками установки, ошибкой нуля первого сочленения и ошибками звена до него.
- {F'} — фактическая система фланца: с ошибками нуля последнего сочленения, звена после него, фланца и крепления щупа.
- Средние ошибки робота (middle errors) — ошибки, которые не входят ни в {B'}, ни в {F'}: ошибки звеньев между первым и последним сочленениями, нулей промежуточных сочленений, податливость, тепловые деформации.

Пусть фактическая поза фланца отличается от номинальной только постоянными множителями слева и справа:

$$
{}^{B}A_{F'}(\bar{q}) = {}^{B}A_{B'}\;{}^{B}A_F(\bar{q})\;{}^{F}A_{F'}
\tag{m.3.1}
$$

где ${}^{B}A_{F'}(\bar{q})$ — фактическая поза фланца при углах сочленений $\bar{q}$;
&emsp;&emsp;${}^{B}A_F(\bar{q})$ — поза фланца по номинальной прямой задаче кинематики (раздел 2.1 теории);
&emsp;&emsp;${}^{B}A_{B'}$ — постоянная матрица преобразования из {B'} в {B};
&emsp;&emsp;${}^{F}A_{F'}$ — постоянная матрица преобразования из {F'} в {F}.

Фактический центр шарика в однородных координатах:

$$
\begin{bmatrix} {}^{B}\bar{p}_{r}^{\,act}(\bar{q}) \\ 1 \end{bmatrix} = {}^{B}A_{B'}\;{}^{B}A_F(\bar{q})\;{}^{F}A_{F'} \begin{bmatrix} {}^{F'}\bar{p}_r \\ 1 \end{bmatrix} = {}^{B}A_{B'} \begin{bmatrix} {}^{B}\bar{p}_r(\bar{q}) \\ 1 \end{bmatrix}
$$

где ${}^{B}\bar{p}_{r}^{\,act}(\bar{q})$ — фактический центр шарика в {B}, мм;
&emsp;&emsp;${}^{F'}\bar{p}_r$ — центр шарика в {F'}, мм;
&emsp;&emsp;${}^{B}\bar{p}_r(\bar{q}) = {}^{B}\bar{p}_F(\bar{q}) + {}^{B}R_F(\bar{q})\,{}^{F}\bar{p}_r$ — рассчитанный центр шарика, в котором ${}^{F}\bar{p}_r$ — образ центра шарика в {F}: $\begin{bmatrix} {}^{F}\bar{p}_r \\ 1 \end{bmatrix} = {}^{F}A_{F'} \begin{bmatrix} {}^{F'}\bar{p}_r \\ 1 \end{bmatrix}$.

Матрица ${}^{B}A_{B'}$ описывает движение твёрдого тела и сохраняет расстояния. Поэтому расстояние от фактического центра шарика до фактического центра сферы:

$$
\left\| {}^{B}\bar{p}_{r}^{\,act}(\bar{q}_i) - {}^{B}\bar{p}_s \right\| = \left\| {}^{B}\bar{p}_r(\bar{q}_i) - {}^{B}\bar{p}_{s}' \right\|, \qquad \begin{bmatrix} {}^{B}\bar{p}_{s}' \\ 1 \end{bmatrix} = \left({}^{B}A_{B'}\right)^{-1} \begin{bmatrix} {}^{B}\bar{p}_s \\ 1 \end{bmatrix}
$$

где ${}^{B}\bar{p}_s$ — фактический центр сферы, мм;
&emsp;&emsp;${}^{B}\bar{p}_{s}'$ — центр сферы, выраженный в {B'} и записанный в координатах {B}, мм.

Уравнения касания (m.2.4.3) выполняются точно при образе центра шарика в {F} и при ${}^{B}\bar{p}_{s}'$. При последующем измерении рассчитанный центр шарика отличается от фактического на одно и то же преобразование ${}^{B}A_{B'}$ во всех позах, как и центр сферы; значит, все рассчитанные точки согласованы между собой в системе {B'}. Итог для фактических поз вида (m.3.1):

$$
\boxed{\varepsilon_i = 0 \text{ при } \begin{bmatrix} {}^{F}\bar{p}_r \\ 1 \end{bmatrix} = {}^{F}A_{F'} \begin{bmatrix} {}^{F'}\bar{p}_r \\ 1 \end{bmatrix},\; \begin{bmatrix} {}^{B}\bar{p}_{s}' \\ 1 \end{bmatrix} = \left({}^{B}A_{B'}\right)^{-1} \begin{bmatrix} {}^{B}\bar{p}_s \\ 1 \end{bmatrix}}
$$

где $\varepsilon_i$ — невязка касания $i$ (m.2.4.3).

| <nobr>Ошибка</nobr>                                                                    | <nobr>Куда</nobr> <nobr>входит</nobr>                                    | <nobr>Последствие</nobr>                                                                         |
| ------------------------------------------------------------------------- | ---------------------------------------------- | ----------------------------------------------------------------------------------- |
| установка основания, звено до первого сочленения, нуль первого сочленения | ${}^{B}A_{B'}$                                 | центр сферы; невязок нет                                                            |
| нуль последнего сочленения, звено после него, фланец, крепление щупа      | ${}^{F}A_{F'}$                                 | центр шарика; найденный центр шарика правилен для номинальной модели; невязок нет   |
| средние ошибки робота                                                     | ни в одну матрицу: ошибка зависит от $\bar{q}$ | невязки и смещение найденных параметров, зависящие от набора поз касаний (раздел 4) |

Численная проверка этого вывода на модели шестизвенного робота — в документе метода. Следствие: точность калибровки центра шарика ограничивают только средние ошибки робота из всех ошибок модели робота.

## 4. Ошибка найденных параметров: случайная и систематическая части

Получим ошибку найденных параметров как сумму случайной и систематической частей и покажем, какую из них оценивает каждый способ проверки по данным калибровки.

### 4.1. Разложение невязки

Невязку касания $i$ при фактических значениях искомых параметров разложим на две части:

$$
\varepsilon_i(\bar{x}^{*}) = \upsilon_i + w_i, \qquad \upsilon_i = E\left[\varepsilon_i(\bar{x}^{*})\right]
\tag{m.4.1.1}
$$

где $\varepsilon_i$ — невязка касания $i$ (m.2.4.3), мм;
&emsp;&emsp;$\bar{x}^{*}$ — фактические значения искомых параметров (центр шарика, центр сферы, расстояние касания); неизвестны;
&emsp;&emsp;$w_i$ — случайная часть невязки (шум): при повторении того же касания принимает другое значение, математическое ожидание 0, мм;
&emsp;&emsp;$\upsilon_i$ — систематическая часть невязки: математическое ожидание невязки, повторяется при повторении того же касания, мм;
&emsp;&emsp;$E[\cdot]$ — математическое ожидание.

Модель касания верна, если $\upsilon_i = 0$ для всех касаний: при фактических параметрах невязка каждого касания состоит только из шума. Это условие нарушают средние ошибки робота (раздел 3), ход до срабатывания, зависящий от направления (раздел 2.5), погрешность формы сферы.

### 4.2. Ошибка параметров

Подставим (m.4.1.1) в первое равенство (m.2.3.9), в котором вектор шума заменён на сумму векторов систематической и случайной частей:

$$
\hat{\bar{x}} - \bar{x}^{*} \approx -\left(J(\bar{x}^{*})^T J(\bar{x}^{*})\right)^{-1} J(\bar{x}^{*})^T \bar{w} \;-\; \left(J(\bar{x}^{*})^T J(\bar{x}^{*})\right)^{-1} J(\bar{x}^{*})^T \bar{\upsilon}
\tag{m.4.2.1}
$$

где $\hat{\bar{x}}$ — вектор параметров, найденный МНК;
&emsp;&emsp;$J(\bar{x}^{*})$ — матрица Якоби невязок (m.2.2.1) при фактических параметрах, размер $m \times n$;
&emsp;&emsp;$\bar{w}$ — вектор-столбец случайных частей невязок $w_i$, размер $m \times 1$, мм;
&emsp;&emsp;$\bar{\upsilon}$ — вектор-столбец систематических частей невязок $\upsilon_i$, размер $m \times 1$, мм;
&emsp;&emsp;$m$ — число касаний, $n$ — число искомых параметров.

Первое слагаемое (m.4.2.1) — случайная ошибка параметров, второе — систематическая. Формула верна в линейном приближении: отклонение параметров от фактических мало по сравнению с $L$.

### 4.3. Ковариация найденных параметров

Пусть величины $w_i$ разных касаний независимы и у всех касаний одно и то же СКО $\sigma$. Тогда ковариационная матрица первого слагаемого (m.4.2.1):

$$
\boxed{\operatorname{Cov}\left(\hat{\bar{x}}\right) \approx \sigma^2 \left(J(\bar{x}^{*})^T J(\bar{x}^{*})\right)^{-1}}
\tag{m.4.3.1}
$$

где $\operatorname{Cov}(\hat{\bar{x}})$ — ковариационная матрица найденных параметров, размер $n \times n$;
&emsp;&emsp;$\sigma$ — СКО шума одного касания; оценивается по невязкам как $s$ (m.2.3.11).

Корни из диагональных элементов (m.4.3.1) — СКО случайной ошибки каждого параметра. Второе слагаемое (m.4.2.1) от $\bar{w}$ не зависит и в (m.4.3.1) не входит. Следовательно, по (m.4.3.1) о систематической ошибке судить нельзя. Отсюда два вопроса: верна ли сама оценка (m.4.3.1) и как обнаружить второе слагаемое (m.4.2.1). Разделы 4.4 и 4.5 отвечают на них по порядку.

### 4.4. Повторение калибровки

При повторении той же калибровки вектор $\bar{w}$ принимает новые значения, а вектор $\bar{\upsilon}$ остаётся прежним (m.4.1.1). Поэтому разброс найденных параметров между повторами — это разброс только первого слагаемого (m.4.2.1). Сравнение этого разброса с (m.4.3.1) проверяет допущения о шуме. Повторение показывает также дрейф между повторами, например тепловой. Постоянную систематическую ошибку повторение не обнаруживает.

### 4.5. Перекрёстная проверка

Касания делятся на две части, I и II, по правилу, выбранному до калибровки. Обозначения раздела:

- $m_{\mathrm{I}}$, $m_{\mathrm{II}}$ — число касаний в частях I и II;
- $J_{\mathrm{I}}(\bar{x}^{*})$, $J_{\mathrm{II}}(\bar{x}^{*})$ — строки матрицы Якоби $J(\bar{x}^{*})$, относящиеся к частям I и II, размеры $m_{\mathrm{I}} \times n$ и $m_{\mathrm{II}} \times n$;
- $\bar{w}_{\mathrm{I}}$, $\bar{w}_{\mathrm{II}}$, $\bar{\upsilon}_{\mathrm{I}}$, $\bar{\upsilon}_{\mathrm{II}}$ — случайные и систематические части невязок частей I и II, вектор-столбцы, мм;
- $\hat{\bar{x}}_{\mathrm{I}}$ — вектор параметров, найденный МНК только по касаниям части I;
- $\bar{\varepsilon}_{\mathrm{II}}(\hat{\bar{x}}_{\mathrm{I}})$ — вектор-столбец невязок касаний части II при параметрах $\hat{\bar{x}}_{\mathrm{I}}$, размер $m_{\mathrm{II}} \times 1$, мм.

Условие: часть I сама определяет все $n$ параметров, то есть $\operatorname{rank} J_{\mathrm{I}}(\bar{x}^{*}) = n$ (m.2.2.19). Обозначим:

$$
U_{\mathrm{I}} = \left(J_{\mathrm{I}}(\bar{x}^{*})^T J_{\mathrm{I}}(\bar{x}^{*})\right)^{-1}
$$

где $U_{\mathrm{I}}$ — матрица размером $n \times n$.

Применим (m.4.2.1) к одной части I:

$$
\hat{\bar{x}}_{\mathrm{I}} - \bar{x}^{*} \approx -U_{\mathrm{I}}\,J_{\mathrm{I}}(\bar{x}^{*})^T\left(\bar{w}_{\mathrm{I}} + \bar{\upsilon}_{\mathrm{I}}\right)
$$

Линеаризуем невязки части II около $\bar{x}^{*}$, затем подставим (m.4.1.1) и предыдущую формулу:

$$
\bar{\varepsilon}_{\mathrm{II}}(\hat{\bar{x}}_{\mathrm{I}}) \approx \bar{\varepsilon}_{\mathrm{II}}(\bar{x}^{*}) + J_{\mathrm{II}}(\bar{x}^{*})\left(\hat{\bar{x}}_{\mathrm{I}} - \bar{x}^{*}\right) = \bar{w}_{\mathrm{II}} + \bar{\upsilon}_{\mathrm{II}} - J_{\mathrm{II}}(\bar{x}^{*})\,U_{\mathrm{I}}\,J_{\mathrm{I}}(\bar{x}^{*})^T\left(\bar{w}_{\mathrm{I}} + \bar{\upsilon}_{\mathrm{I}}\right)
$$

Сгруппируем случайные и систематические слагаемые:

$$
\bar{\varepsilon}_{\mathrm{II}}(\hat{\bar{x}}_{\mathrm{I}}) \approx \underbrace{\bar{w}_{\mathrm{II}} - J_{\mathrm{II}}(\bar{x}^{*})\,U_{\mathrm{I}}\,J_{\mathrm{I}}(\bar{x}^{*})^T\,\bar{w}_{\mathrm{I}}}_{\text{случайная часть}} \;+\; \underbrace{\bar{\upsilon}_{\mathrm{II}} - J_{\mathrm{II}}(\bar{x}^{*})\,U_{\mathrm{I}}\,J_{\mathrm{I}}(\bar{x}^{*})^T\,\bar{\upsilon}_{\mathrm{I}}}_{\text{систематическая часть}}
\tag{m.4.5.1}
$$

Что даёт один только шум. Пусть $\bar{\upsilon}_{\mathrm{I}} = \bar{0}$ и $\bar{\upsilon}_{\mathrm{II}} = \bar{0}$. Векторы $\bar{w}_{\mathrm{I}}$ и $\bar{w}_{\mathrm{II}}$ независимы, их средние равны 0, СКО каждого касания $\sigma$. Поэтому математическое ожидание суммы квадратов невязок части II — сумма вклада шума части II, $\sigma^2 m_{\mathrm{II}}$, и вклада шума части I через найденные параметры:

$$
\sigma^2 \operatorname{tr}\left(J_{\mathrm{II}}(\bar{x}^{*})\,U_{\mathrm{I}}\,J_{\mathrm{I}}(\bar{x}^{*})^T J_{\mathrm{I}}(\bar{x}^{*})\,U_{\mathrm{I}}\,J_{\mathrm{II}}(\bar{x}^{*})^T\right) = \sigma^2 \operatorname{tr}\left(J_{\mathrm{II}}(\bar{x}^{*})\,U_{\mathrm{I}}\,J_{\mathrm{II}}(\bar{x}^{*})^T\right)
$$

где $\operatorname{tr}$ — след матрицы.

$$
\boxed{E\left[\left\lVert \bar{\varepsilon}_{\mathrm{II}}(\hat{\bar{x}}_{\mathrm{I}}) \right\rVert^2\right] = \sigma^2\left(m_{\mathrm{II}} + \operatorname{tr}\left(J_{\mathrm{II}}(\bar{x}^{*})\,U_{\mathrm{I}}\,J_{\mathrm{II}}(\bar{x}^{*})^T\right)\right)}
\tag{m.4.5.2}
$$

Сумма квадратов невязок части II, превышающая (m.4.5.2) больше, чем допускает её разброс при одном шуме, указывает на систематическую часть. Граница этого разброса с заданной вероятностью ложной тревоги не выведена.

Какую систематическую часть такая проверка обнаруживает. Пусть систематическая часть каждой части касаний записывается через свой постоянный вектор:

$$
\bar{\upsilon}_{\mathrm{I}} = J_{\mathrm{I}}(\bar{x}^{*})\,\bar{\gamma}_{\mathrm{I}}, \qquad \bar{\upsilon}_{\mathrm{II}} = J_{\mathrm{II}}(\bar{x}^{*})\,\bar{\gamma}_{\mathrm{II}}
$$

где $\bar{\gamma}_{\mathrm{I}}$, $\bar{\gamma}_{\mathrm{II}}$ — постоянные векторы-столбцы размером $n \times 1$, в единицах параметров.

Подставим их в систематическую часть (m.4.5.1); произведение $U_{\mathrm{I}}\,J_{\mathrm{I}}(\bar{x}^{*})^T J_{\mathrm{I}}(\bar{x}^{*})$ — единичная матрица:

$$
J_{\mathrm{II}}(\bar{x}^{*})\,\bar{\gamma}_{\mathrm{II}} - J_{\mathrm{II}}(\bar{x}^{*})\,U_{\mathrm{I}}\,J_{\mathrm{I}}(\bar{x}^{*})^T J_{\mathrm{I}}(\bar{x}^{*})\,\bar{\gamma}_{\mathrm{I}} = J_{\mathrm{II}}(\bar{x}^{*})\left(\bar{\gamma}_{\mathrm{II}} - \bar{\gamma}_{\mathrm{I}}\right)
$$

где $\bar{\gamma}_{\mathrm{I}}$, $\bar{\gamma}_{\mathrm{II}}$ — векторы предыдущей формулы.

- Если $J_{\mathrm{II}}(\bar{x}^{*})(\bar{\gamma}_{\mathrm{II}} - \bar{\gamma}_{\mathrm{I}}) \ne \bar{0}$, систематическая часть остаётся в невязках части II и обнаруживается, если превышает случайную часть.
- Если $\bar{\gamma}_{\mathrm{I}} = \bar{\gamma}_{\mathrm{II}} = \bar{\gamma}$, то есть на всех касаниях $\bar{\upsilon} = J(\bar{x}^{*})\,\bar{\gamma}$, систематическая часть невязок части II равна нулю при любом разбиении. По (m.4.2.1) такая систематическая часть сдвигает найденные параметры на $-\bar{\gamma}$ и не оставляет следа ни в каких невязках. Пример — постоянная и линейная по направлению подхода составляющие хода до срабатывания (раздел 2.5).
- Систематическая часть, которую нельзя записать в виде $J(\bar{x}^{*})\,\bar{\gamma}$, в непредставимой доле остаётся в невязках оценивания по всем касаниям — продолжение второго равенства (m.2.3.9). Её видно и без разбиения.

Вывод: перекрёстная проверка добавляет к анализу невязок по всем касаниям обнаружение систематической части, для которой разным частям касаний нужны разные векторы $\bar{\gamma}$. Систематическую часть вида $J(\bar{x}^{*})\,\bar{\gamma}$ с одним $\bar{\gamma}$ на всех касаниях по данным этой калибровки обнаружить нельзя; её величину определяют только независимым измерением или расчётом по модели вызывающего её фактора (раздел 4.6).

### 4.6. Расчёт систематической ошибки по модели фактора

Получим систематическую ошибку параметров, вызванную фактором влияния, через величину этого фактора. Пусть фактор описывается вектором величин $\bar{\phi}$, и систематическая часть невязок линейно зависит от него (отклонения малы по сравнению с $L$):

$$
\bar{\upsilon} = D(\bar{x}^{*})\,\bar{\phi}
\tag{m.4.6.1}
$$

где $\bar{\phi}$ — вектор-столбец величин фактора размером $n_{\phi} \times 1$; например, отклонения параметров модели робота от номинальных (длины звеньев, нули сочленений) или размах хода до срабатывания;
&emsp;&emsp;$D(\bar{x}^{*})$ — матрица чувствительности невязок касаний к величинам фактора, размер $m \times n_{\phi}$; вычисляется по номинальной модели в позах касаний;
&emsp;&emsp;$n_{\phi}$ — число величин фактора.

Подставим (m.4.6.1) во второе слагаемое (m.4.2.1):

$$
\boxed{\Delta\hat{\bar{x}} = -\left(J(\bar{x}^{*})^T J(\bar{x}^{*})\right)^{-1} J(\bar{x}^{*})^T D(\bar{x}^{*})\,\bar{\phi}}
\tag{m.4.6.2}
$$

где $\Delta\hat{\bar{x}}$ — систематическая ошибка найденных параметров, вызванная фактором.

Если известна только ковариационная матрица величин фактора $\Sigma_{\phi}$ размером $n_{\phi} \times n_{\phi}$, ковариационная матрица систематической ошибки равна произведению матрицы перед $\bar{\phi}$ в (m.4.6.2), матрицы $\Sigma_{\phi}$ и транспонированной матрицы перед $\bar{\phi}$. Обе формы вычисляются без случайных испытаний; Монте-Карло нужен только для проверки линеаризации полной нелинейной моделью.

Пример — ход до срабатывания, известный с точностью до размаха $\zeta$: $\delta_p(\tau_i, \psi_i) = \zeta\,g(\tau_i, \psi_i)$, где $g$ — известная безразмерная форма зависимости от направления. Касание с большим ходом до срабатывания регистрируется ближе к центру сферы, поэтому $\upsilon_i = -\zeta\,g(\tau_i, \psi_i)$, и по (m.4.6.2) систематическая ошибка параметров пропорциональна $\zeta$ с коэффициентом, который зависит только от плана касаний.

Модели факторов из карточек анализа литературы, которые записываются в форме (m.4.6.1). Для фактора, который действует через ход до срабатывания, систематическая часть невязки касания $i$ равна ходу до срабатывания этого касания со знаком минус (раздел 2.5 и пример выше), и столбец $D(\bar{x}^{*})$ для величины фактора — производная хода до срабатывания по ней со знаком минус. Для фактора, который действует через ошибку робота, столбец — проекция на орт внешней нормали сферы в касании $i$ (2.2.1) отклонения фактического центра шарика от рассчитанного при единичном значении величины фактора.

| <nobr>Фактор</nobr>                                     | <nobr>Модель</nobr> <nobr>и</nobr> <nobr>место</nobr>                                                                                                                | <nobr>Величины</nobr> $\bar{\phi}$, <nobr>по</nobr> <nobr>которым</nobr> <nobr>модель</nobr> <nobr>линейна</nobr> | <nobr>Величины,</nobr> <nobr>заданные</nobr> <nobr>отдельно</nobr> <nobr>или</nobr> <nobr>входящие</nobr> <nobr>нелинейно</nobr> |
| ------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| ФВ9, ФВ14                     | L1: ход до срабатывания пропорционален силе срабатывания (раздел 3.3.1 анализа литературы)                                                                                              | $c_{st}$                                                                                                          | сила срабатывания $Q_{trig}(\omega)$ — из геометрии опор и силы пружины                                                          |
| ФВ9                                         | L4: треугольник по азимуту и наклон к оси стилуса (раздел 3.3.3 анализа литературы)                                                                                                     | $\delta_{p.v}$, амплитуда треугольника $\delta_{p.h}$                                                             | форма $\xi(\tau)$ и поворот треугольника                                                                                         |
| ФВ9                                         | L5, L6: сила срабатывания с весом держателя и стилуса (разделы 3.3.4, 3.3.5 анализа литературы)                                                                                         | $c_{st}$, $\delta_{p.g}(\kappa)$                                                                                  | эффективный радиус сечения стилуса, угол к силе тяжести                                                                          |
| ФВ9, ФВ14, ФВ22 | L7: ход контакта, упругая деформация стилуса, контактная деформация (раздел 3.3.6 анализа литературы)                                                                                   | $c_{st}$, $c_{ax}$                                                                                                | $\delta_{p.sw}$, $\delta_{p.hz}$ — по расчёту                                                                                    |
| ФВ9                                         | L11: поворот держателя вокруг оси через две опоры (раздел 3.3.10 анализа литературы)                                                                                                    | $\delta_{p.r}$, $\delta_{p.b}$                                                                                    | $\omega_0$, $\Lambda$; модель для $\tau > 11°$                                                                                   |
| ФВ9                                         | L13: карта хода до срабатывания по направлениям (раздел 3.3.12 анализа литературы)                                                                                                      | значения карты в узлах                                                                                            | способ интерполяции между узлами                                                                                                 |
| ФВ9, ФВ15, ФВ23 | L9: путь машины после срабатывания (раздел 3.3.8 анализа литературы)                                                                                                                    | $t_{lat}$, $1/\dot{v}_{dec}$                                                                                      | скорость $v_s$ — из плана касаний; при одной скорости всех касаний слагаемое постоянно и поглощается $\rho$ (раздел 2.4) |
| ФВ3                                         | модифицированные параметры Денавита — Хартенберга с отклонениями (L19, L20, L22, L23; разделы 3.4.5, 3.4.6, 3.4.8, 3.5.2 анализа литературы)                                            | отклонения длин и углов звеньев и нулей промежуточных сочленений                                                  | —                                                                                                                                |
| ФВ4                                         | поворот сочленения равен податливости, умноженной на момент силы тяжести (L20, L22, L23); коэффициенты при функциях углов (L25) (разделы 3.4.6, 3.4.8, 3.5.2, 3.5.4 анализа литературы) | податливости сочленений или коэффициенты L25                                                                      | массы и центры масс звеньев и щупа — для L20, L22, L23                                                                           |
| ФВ4                                         | смещение под силой — матрица Якоби робота, податливости сочленений и транспонированная матрица Якоби, умноженные на силу (L26, раздел 3.5.5 анализа литературы)                         | податливости сочленений                                                                                           | направление и величина силы касания                                                                                              |
| ФВ5                                         | тепловые отклонения параметров $d_1$, $a_2$, $d_4$, $d_6$ и угла между осями 2 и 3 (L25, раздел 3.5.4 анализа литературы)                                                               | эти отклонения в момент касания                                                                                   | зависимость от времени и температуры — эмпирическая                                                                              |
| ФВ6                                         | люфт — разность углов сочленения при подходе с разных сторон (L22, L23; разделы 3.4.8, 3.5.2 анализа литературы)                                                                        | половина люфта сочленения                                                                                         | знак — по направлению последнего движения сочленения (вывод автора)                                                              |

Остальные факторы в (m.4.6.1) не входят: ФВ1 и ФВ2 невязок не дают (раздел 3); ФВ7, ФВ8, повторяемость хода до срабатывания (ФВ9) и случайная часть задержки сигнала (ФВ15) — случайная часть невязок и ковариация (m.4.3.1); ФВ17 поглощается $\rho$ (раздел 2.4); для ФВ18 источники дают только пределы формы, моделей нет; ФВ20 задаёт позы касаний, по которым вычисляются $J(\bar{x}^{*})$ и $D(\bar{x}^{*})$; ФВ21 относится к позам последующих измерений, а не к невязкам калибровки.

## 5. Задержка сигнала

Задача раздела — описать, из чего складывается задержка сигнала, нужна ли она для калибровки и как её измерить.

Задержка сигнала $t_{lat}$ складывается из задержек звеньев пути сигнала срабатывания от выхода щупа до системы управления робота: фильтры входов промежуточных устройств, циклы их программ, выходы, фильтр входа системы управления, передача по шине и опрос входа с периодом $T_{in}$. Величины звеньев конкретной установки — в документе метода.

| <nobr>Свойство</nobr>                                       | <nobr>Значение</nobr>                                                                                                                                          | <nobr>Основание</nobr>                                                                                                    |
| ---------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------ |
| средняя задержка зависит от ориентации касания | нет                                                                                                                                               | допущение: всё, что зависит от направления нормальной силы в точке касания, происходит до срабатывания и входит в ход до срабатывания |
| случайная часть                                | от опроса входа: равномерно от 0 до $T_{in}$, перебег шарика от 0 до $v_s T_{in}$, СКО $v_s T_{in}/\sqrt{12}$; плюс циклы промежуточных устройств | для отдельного касания не определяется                                                                       |
| нужна для калибровки центра шарика             | нет, если расстояние касания оценивается как искомый параметр и скорость поиска одинакова                                                         | перебег шарика за время задержки входит в $\rho$ (раздел 2.4)                                                |
| нужна для                                      | тормозного пути после касания; измерений поверхностей                                                                                             | при измерении поверхности оценить расстояние касания нельзя: эталонной сферы нет                             |

Способы измерить среднюю задержку сигнала:

| <nobr>Способ</nobr>                                                                     | <nobr>Что</nobr> <nobr>охватывает</nobr>               | <nobr>Что</nobr> <nobr>нужно</nobr>                                                                                                                                    |
| -------------------------------------------------------------------------- | ---------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------- |
| 1. Две скорости поиска                                                     | всю цепь                     | серии касаний одной точки при одной ориентации щупа на скоростях $v_1$ и $v_2$; номер цикла системы управления, в котором обнаружено касание |
| 2. Осциллограф: фронт сигнала на выходе щупа и на входе системы управления | цепь без шины и опроса входа | осциллограф                                                                                                                                  |

Способ 1: для одной точки при одной ориентации координата касания $p_a(t_c)$ и ход до срабатывания $\delta_p$ одинаковы при обеих скоростях, поэтому при вычитании (m.1.1) при $v_1$ из (m.1.1) при $v_2$ они сокращаются:

$$
\begin{gathered}
p_{a.det}(v_1) = p_a(t_c) + \delta_p + v_1\,t_{lat},
\qquad
p_{a.det}(v_2) = p_a(t_c) + \delta_p + v_2\,t_{lat}
\end{gathered}
$$

$$
\boxed{t_{lat} = \frac{p_{a.det}(v_2) - p_{a.det}(v_1)}{v_2 - v_1}}
$$

где $p_{a.det}(v_1)$, $p_{a.det}(v_2)$ — средние зарегистрированные координаты при скоростях поиска $v_1$ и $v_2$, мм;
&emsp;&emsp;$p_a(t_c)$ — координата TCP в момент касания, мм;
&emsp;&emsp;$\delta_p$ — ход до срабатывания, мм;
&emsp;&emsp;$t_{lat}$ — средняя задержка сигнала, с.

## 6. Учёт различий хода до срабатывания между парами $(\tau, \psi)$

Задача раздела — устранить 2-е и 4-е слагаемые хода до срабатывания (m.2.5.1): смещение центра шарика на вектор $\bar{u}$ и систематические невязки $\eta_i$.

| <nobr>Вариант</nobr>     | <nobr>Суть</nobr>                                                                                   | <nobr>Достоинство</nobr>                   | <nobr>Недостаток</nobr>                                                                                                     |
| ----------- | -------------------------------------------------------------------------------------- | ----------------------------- | -------------------------------------------------------------------------------------------------------------- |
| 0. Проверка | калибровка с оцениваемым расстоянием касания, анализ невязок                           | без дополнительных измерений  | только решает, нужна ли поправка; 2-е слагаемое в невязках не видно (раздел 4.5)                               |
| 1           | одна пара $(\tau, \psi)$ на все касания                                                | ход до срабатывания одинаков  | часть точек сферы может оказаться недостижимой при одной паре                                                  |
| 2           | таблица расстояний касания $\rho(\tau, \psi)$ по всем парам плана, отдельное измерение | учитывает все различия        | дополнительное измерение; требования ниже                                                                      |
| 3           | классы касаний по $(\tau, \psi)$, своё расстояние касания у каждого класса             | без дополнительного измерения | на каждый класс нужно достаточно касаний; каждый новый искомый параметр ослабляет идентифицируемость остальных |

Вариант 2 соответствует калибровке по каждому используемому направлению; для кинематических резистивных щупов Renishaw источник L1 указывает, что она «может потребоваться», если ошибка измерения от разброса хода до срабатывания больше допустимой погрешности измерения (L1, раздел 3.3.1).

Критерий варианта 0: различиями можно пренебречь, если проверка 1 (раздел 2.3.5) даёт $s / \sigma_0 \le 1{,}30$ и проверку 3 (раздел 2.5) проходят все пары с несколькими касаниями. Пары с одним касанием этим критерием не проверяются.

В варианте 2 ход до срабатывания каждой пары измеряется так, чтобы ошибка центра шарика не влияла на результат: ориентация щупа не меняется, поэтому сдвиг центра шарика одинаков у всех касаний и уходит в центр сферы (условие 2 раздела 2.2.5).

| <nobr>Шаг</nobr> | <nobr>Действие</nobr>                                                                                                                                                                              |
| --- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1   | щуп в одной ориентации; ориентация не меняется до конца измерения                                                                                                                     |
| 2   | для каждой пары $k = 1 \ldots K$ шарик подводится к сфере так, чтобы направление подхода в {P} было таким же, как у пары $k$ при калибровке; оно отклонено от оси стилуса на $\tau_k$ |
| 3   | несколько повторов на пару, скорость поиска как при калибровке                                                                                                                        |
| 4   | оценивание по МНК центра сферы и расстояний касания $\rho_1 \ldots \rho_K$                                                                                                            |
| 5   | в уравнении касания $i$ при калибровке вместо $L$ или $\rho$ стоит табличное расстояние для его пары $(\tau_i, \psi_i)$                                                               |

Шаг 4:

$$
\min_{{}^{B}\bar{p}_s,\,\rho_1 \ldots \rho_K} \sum_{k=1}^{K}\sum_{h} \left( \left\|{}^{B}\bar{p}_r(\bar{q}_{kh}) - {}^{B}\bar{p}_s\right\| - \rho_k \right)^2,
\qquad
\sum_{k=1}^{K} \left(\rho_k - \rho_0\right) {}^{P}\bar{e}_a(\tau_k,\psi_k) = \bar{0}
$$

где ${}^{B}\bar{p}_s$, ${}^{B}\bar{p}_r(\bar{q}_{kh})$ — центр сферы и рассчитанный центр шарика, как в 6.1;
&emsp;&emsp;$K$ — число пар $(\tau, \psi)$ плана калибровки;
&emsp;&emsp;$\bar{q}_{kh}$ — углы сочленений в повторе $h$ пары $k$; центр шарика рассчитывается по ним с его начальным приближением;
&emsp;&emsp;$\rho_k$ — расстояние касания пары $k$, мм;
&emsp;&emsp;$\rho_0$ — среднее всех $\rho_k$, мм;
&emsp;&emsp;${}^{P}\bar{e}_a(\tau_k,\psi_k)$ — орт направления подхода пары $k$ в {P} (2.2.4);
&emsp;&emsp;$\bar{0}$ — нулевой вектор.

Второе условие нужно потому, что сдвиг центра сферы на вектор меняет каждое $\rho_k$ на проекцию этого вектора на направление подхода пары $k$: часть $\rho_k$, линейная по направлению подхода, неидентифицируема (то же, что 3-е слагаемое (m.2.5.1)), и её принимают равной нулю.

Шаг 5:

$$
\left\| {}^{B}\bar{p}_F(\bar{q}_i) + {}^{B}R_F(\bar{q}_i)\,{}^{F}\bar{p}_r - {}^{B}\bar{p}_s \right\| = \rho(\tau_i,\psi_i)
$$

где $\rho(\tau_i,\psi_i)$ — табличное расстояние касания для пары $(\tau_i, \psi_i)$, мм.

Ошибки робота варианту 2 не мешают в той мере, в какой ошибка положения фланца постоянна в области движения при постоянной ориентации: тогда неточность начального центра шарика и ошибка робота сдвигают все рассчитанные центры шарика одинаково и уходят в центр сферы, а не в расстояния касания.

| <nobr>Условие</nobr> <nobr>варианта</nobr> 2                                                                  | <nobr>Статус</nobr>                                                                                                                 |
| ----------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------- |
| знать радиусы шарика и сферы, ход до срабатывания и задержку сигнала по отдельности | не нужно: всё входит в табличные расстояния касания                                                                    |
| та же сфера и та же скорость поиска при калибровке                                  | требуется                                                                                                              |
| малая погрешность формы сферы                                                       | требуется: отклонение формы в точке касания будет принято за различие хода до срабатывания                             |
| ход до срабатывания не зависит от направления силы тяжести относительно щупа        | допущение: при измерении таблицы щуп в одной ориентации, при калибровке — в разных; к анализу — L5 (раздел 3.1 анализа литературы) |
| касания под другими $(\tau, \psi)$                                                  | таблица верна только для $K$ пар; нужна более густая таблица или касания только под измеренными парами                 |

## 7. Что содержит КТК

Линеаризуем (4.3.1) около фактических значений, как в разделе 4.1. Фактический центр шарика в позе $\bar{q}_i$ отличается от рассчитанного по найденным параметрам на три слагаемых: ошибку найденного центра шарика, повёрнутую в {B}, ошибку робота в этой позе и ошибку найденного центра поверочной сферы:

$$
\varepsilon_{\mathrm{ver},i} \approx {}^{B}\bar{e}_{n,i}^{\,T}\left({}^{B}R_F(\bar{q}_i)\,\Delta{}^{F}\bar{p}_r + {}^{B}\bar{b}(\bar{q}_i) - \Delta{}^{B}\bar{p}_{\mathrm{ver}}\right) + \rho_{\mathrm{ver},i} - \left(\hat{\rho} - r_s + r_{\mathrm{ver}}\right)
\tag{m.7.1}
$$

где ${}^{B}\bar{e}_{n,i}$ — орт внешней нормали поверочной сферы в точке проверочного касания $i$, выражен в {B} (2.2.1);
&emsp;&emsp;$\Delta{}^{F}\bar{p}_r = {}^{F}\hat{\bar{p}}_r - {}^{F}\bar{p}_r$ — ошибка найденного центра шарика относительно его образа в {F} (раздел 3), мм;
&emsp;&emsp;${}^{B}\bar{b}(\bar{q}_i)$ — ошибка рассчитанного центра шарика от средних ошибок робота в позе $\bar{q}_i$: рассчитанный по номинальной модели минус фактический, выражена в {B}, мм;
&emsp;&emsp;$\Delta{}^{B}\bar{p}_{\mathrm{ver}}$ — ошибка найденного центра поверочной сферы; в неё входят и постоянные ошибки {B'} (раздел 3), мм;
&emsp;&emsp;$\rho_{\mathrm{ver},i}$ — фактическое расстояние касания проверочного касания $i$: сумма радиусов шарика и поверочной сферы минус ход до срабатывания и перебег за задержку этого касания (раздел 2.5), мм.

Следствия (m.7.1):

| <nobr>Слагаемое</nobr> | <nobr>Что</nobr> <nobr>попадает</nobr> в <nobr>КТК</nobr> |
| --- | --- |
| $\Delta{}^{F}\bar{p}_r$ | только через различие ${}^{B}R_F(\bar{q}_i)$ между ориентациями: при одной ориентации слагаемое постоянно и уходит в $\Delta{}^{B}\bar{p}_{\mathrm{ver}}$. При двух ориентациях не проявляется составляющая $\Delta{}^{F}\bar{p}_r$ вдоль оси поворота от одной ориентации к другой; она не искажает и измерения в этих двух ориентациях. Поэтому ориентации проверочных касаний — те же, что в последующих измерениях (теория, раздел 4.1, требование 2) |
| ${}^{B}\bar{b}(\bar{q}_i)$ | средние ошибки робота в позах проверки — полностью, кроме части, общей для всех касаний. Метод, который находит только центр шарика, не делает $\chi_{95}$ меньше вклада этого слагаемого; метод, который учитывает и ошибки робота, может сделать |
| $\rho_{\mathrm{ver},i} - (\hat{\rho} - r_s + r_{\mathrm{ver}})$ | постоянная часть — ошибка размера (теория, раздел 4.1, требование 5); часть, зависящая от направления и ориентации щупа, — разброс хода до срабатывания (ФВ9) и задержки сигнала (ФВ15) |
| шум касаний | случайная часть невязок проверочных касаний — повторяемость робота (ФВ7) и щупа (ФВ9, ход до срабатывания); она же входит и в последующие измерения |

Для анализа на шагах 4 и 5 вместе с $\chi_{95}$ вычисляются две составляющие: среднее $\varepsilon_{\mathrm{ver},i}$ — ошибка размера, и размах центров поверочной сферы, найденных МНК отдельно по касаниям каждой ориентации, — параметр местоположения PLT по L28 (раздел 3.6.2 анализа литературы). КТК остаётся $\chi_{95}$.

## 8. Потенциальные факторы влияния на основе моделей

Факторы влияния этого раздела выведены из моделей разделов 1–6, а не из шага 4 порядка исследования (теория, раздел 1): это потенциальные факторы, которые следуют из принятых в моделях допущений. Степень влияния ни одного фактора не оценена; способ оценки — (m.4.6.2) по модели фактора и проверки разделов 2.3, 2.5, 4.4, 4.5 по данным.

| <nobr>ФВ</nobr>   | <nobr>Фактор</nobr> | <nobr>Где</nobr> <nobr>входит</nobr> в <nobr>ММК</nobr>                      | <nobr>Куда</nobr> <nobr>уходит</nobr> <nobr>при</nobr> <nobr>оценивании</nobr>                                  | <nobr>Данные</nobr> <nobr>источников</nobr> |
| ---- | ------------------------------------------------------------ | ------------------------------------- | ----------------------------------------------------------- | ----------------------------------------------------------------------- |
| ФВ1  | ошибки основания робота и нуля первого сочленения            | ${}^{B}A_{B'}$ (m.3.1)                  | центр сферы; невязок нет                                    | раздел 3.7.2 анализа литературы, ФВ1 |
| ФВ2  | ошибки фланца, крепления щупа и нуля последнего сочленения   | ${}^{F}A_{F'}$ (m.3.1)                  | центр шарика; невязок нет                                   | раздел 3.7.2 анализа литературы, ФВ2 |
| ФВ3  | средние ошибки робота: звенья, нули промежуточных сочленений | систематическая часть невязок (m.4.6.1) | смещение параметров (m.4.6.2) и невязки                       | раздел 3.7.2 анализа литературы, ФВ3 |
| ФВ4  | податливость робота                                          | зависит от позы                       | как средние ошибки                                          | раздел 3.7.2 анализа литературы, ФВ4 |
| ФВ5  | тепловые деформации                                          | меняются во время калибровки          | как средние ошибки, плюс дрейф между повторами (раздел 4.4) | раздел 3.7.2 анализа литературы, ФВ5 |
| ФВ7  | повторяемость робота при записи фактических углов            | случайная часть невязок               | ковариация (m.4.3.1)                                          | раздел 3.7.2 анализа литературы, ФВ7 |
| ФВ9  | повторяемость щупа                                           | случайная часть невязок               | ковариация (m.4.3.1)                                          | раздел 3.7.2 анализа литературы, ФВ9 |
| ФВ8  | разрешение передаваемых углов сочленений                     | случайная часть невязок               | ковариация (m.4.3.1)                                          | раздел 3.7.2 анализа литературы, ФВ8 |
| ФВ9  | средний ход до срабатывания                                  | $\rho$ (раздел 2.4)                   | $\rho$                                                      | раздел 3.7.2 анализа литературы, ФВ9 |
| ФВ9  | разброс хода до срабатывания по направлениям                 | (m.2.5.1)                               | линейная часть — центр шарика, остаток $\eta_i$ — невязки   | раздел 3.7.2 анализа литературы, ФВ9 |
| ФВ9  | зависимость хода до срабатывания от силы тяжести             | меняется с ориентацией щупа           | как разброс по направлениям                                 | раздел 3.7.2 анализа литературы, ФВ9 |
| ФВ15 | средний перебег шарика за задержку сигнала                   | $\rho$ (раздел 2.4)                   | $\rho$, если скорость поиска одинакова                      | раздел 3.7.2 анализа литературы, ФВ15 |
| ФВ15 | случайная часть задержки, опрос входа                        | случайная часть невязок               | ковариация (m.4.3.1)                                          | раздел 3.7.2 анализа литературы, ФВ15 |
| ФВ17 | радиусы шарика и сферы                                       | $\rho$                                | $\rho$                                                      | раздел 3.7.2 анализа литературы, ФВ17 |
| ФВ18 | погрешность формы сферы                                      | систематическая часть невязок         | невязки                                                     | раздел 3.7.2 анализа литературы, ФВ18 |

Факторы ФВ6, зависимость ФВ9 (ход до срабатывания) от скорости, ФВ14, ФВ19–ФВ23 в таблицу не внесены: их место в моделях не определено; их величины — в разделе 3.7.2 анализа литературы.

## 9. Выводы

| №   | <nobr>Вопрос</nobr>                                             | <nobr>Вывод</nobr>                                                                                                                                                                                                                                                                                                                                                  | <nobr>Раздел</nobr>      |
| --- | -------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------- |
| 1   | модель регистрации касания                         | зарегистрированная координата TCP отличается от координаты касания на ход до срабатывания $\delta_p$ и перебег шарика за время задержки $v_s\,t_{lat}$, формула (m.1.1)                                                                                                                                                                                  | 1           |
| 2   | уравнения калибровки и однозначность центра шарика | нормальные уравнения нелинейного МНК (m.2.2.12) решаются методом Гаусса — Ньютона; приращение единственно при $\operatorname{rank} J = n$ (m.2.2.19); необходимые условия на набор касаний — 1–3 (раздел 2.2.5) и 4 (раздел 2.4)                                                                                                                           | 2.1, 2.2    |
| 3   | невязки и шум                                      | СКО шума одного касания оценивается по невязкам с делителем $m - n$ (m.2.3.11); проверки 1–3 отличают шум от выбросов и неучтённых явлений                                                                                                                                                                                                               | 2.3         |
| 4   | что искажает центр шарика                          | при оцениваемом $\rho$ (m.2.4.3) радиусы шарика и сферы, средний ход до срабатывания и перебег за среднюю задержку центр шарика не искажают; разброс хода по направлениям смещает центр шарика своей линейной частью $\bar{u}$ (m.2.5.1); из ошибок робота искажают только средние ошибки                                                                  | 2.4, 2.5, 3 |
| 5   | что можно узнать по данным калибровки              | ковариация (m.4.3.1) — только случайная ошибка; повторение — проверка допущений о шуме и дрейф; перекрёстная проверка — систематическая часть, различная для частей касаний; систематическая часть вида $J(\bar{x}^{*})\,\bar{\gamma}$ по данным калибровки не обнаруживается и определяется только по модели фактора (m.4.6.2) или независимым измерением | 4           |
| 6   | задержка сигнала и различия хода до срабатывания   | при оцениваемом $\rho$ и одинаковой скорости поиска задержка для калибровки не нужна; различия хода до срабатывания учитываются вариантами раздела 6                                                                                                                                                                                                   | 5, 6        |
